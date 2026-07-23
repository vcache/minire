#include <scene-impl.hpp>

#include <rasterizer.hpp>
#include <rasterizer/constants.hpp>
#include <scene-impl/gltf-instantiator.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/scene/spatial-index/brute-force.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/geometry.hpp>
#include <minire/utils/overloaded.hpp>

#include <fmt/ranges.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <type_traits>
#include <vector>

namespace minire
{
    static constexpr size_t kNoDepth = std::numeric_limits<size_t>::max();

    // OpbIdHolder //

    class SceneImpl::OpbIdHolder
    {
        OpbIdHolder(OpbIdHolder const &) = delete;
        OpbIdHolder& operator=(OpbIdHolder const &) = delete;
        OpbIdHolder(OpbIdHolder &&) = delete;
        OpbIdHolder& operator=(OpbIdHolder &&) = delete;

    public:
        explicit OpbIdHolder(SceneImpl & scene)
            : _scene(scene)
            , _id(_scene.allocateOpbId())
        {}

        ~OpbIdHolder()
        {
            _scene.releaseOpbId(_id);
        }

        SceneImpl::OpbId id() const { return _id; }

    private:
        SceneImpl            & _scene;
        SceneImpl::OpbId const _id;
    };

    // SceneImpl::Leaf //

    template<typename Derived, typename ObjectType>
    SceneImpl::Leaf<Derived, ObjectType>::Leaf(std::string name,
                                               typename ObjectType::ModelType const & model,
                                               std::shared_ptr<Node> const & parent,
                                               SceneImpl & scene)
        : ObjectType(std::move(name), model, ObjectType::kNoFlags)
        , _parent(parent)
        , _depth(parent && parent->_depth != kNoDepth ? parent->_depth + 1 : kNoDepth)
        , _scene(scene)
    {}

    template<typename Derived, typename ObjectType>
    void SceneImpl::Leaf<Derived, ObjectType>::setParent(scene::Node::Sptr const & newParentIface)
    {
        _scene.setParent(*this, newParentIface);
        ObjectType::invalidate();
    }

    template<typename Derived, typename ObjectType>
    void SceneImpl::Leaf<Derived, ObjectType>::invalidate(ObjectType::Mask mask)
    {
        if (mask)
        {
            assert(mask & (ObjectType::kBaseMask | kParentTransformChanged | kActiveLerp));
            _scene.activateLeaf(this, mask);
        }
    }

    template<typename Derived, typename ObjectType>
    models::ScenePath SceneImpl::Leaf<Derived, ObjectType>::absPath() const
    {
        models::ScenePath result;

        if (auto parent = _parent.lock(); parent)
        {
            result = parent->absPath();
        }

        result.push_back(name());
        return result;
    }

    template<typename Derived, typename ObjectType>
    SceneImpl::Leaf<Derived, ObjectType>::~Leaf()
    {
        if (ObjectType::invalidated())
        {
            _scene.eraseLeaf(this);
        }
    }

    // SceneImpl::*Leaf //

    SceneImpl::MeshLeaf::MeshLeaf(std::string name,
                                  models::Mesh && model,
                                  std::shared_ptr<Node> const & parent,
                                  std::shared_ptr<rasterizer::Mesh> const & mesh,
                                  SceneImpl & scene)
        : Leaf(std::move(name), std::move(model), parent, scene)
        , _mesh(mesh)
        , _opbId(scene._enableOpb // OpbId is always allocated (even if disabled in a model)
                    ? std::make_unique<SceneImpl::OpbIdHolder>(scene)
                    : std::unique_ptr<SceneImpl::OpbIdHolder>())
        , _spatialHandler(*scene._spatialIndex, this, kMeshLayer)
    {}

    SceneImpl::OpbId SceneImpl::MeshLeaf::opbId() const
    {
        return _scene._enableOpb && enableOpb() && _opbId ? _opbId->id() : 0;
    }

    void SceneImpl::MeshLeaf::revalidate(Mask mask)
    {
        auto p = _parent.lock();
        if (OpbId const id = opbId();
            invalidatedAny(mask & kOutline) && id != 0)
        {
            models::Outline const & effectiveOutline =
                p && std::holds_alternative<std::monostate>(outline()) ? p->_effectiveOutline
                                                                       : outline();

            std::visit(utils::Overloaded
            {
                [this, id](std::monostate const &)
                {
                    _scene._pixelEdgeOutlines.erase(id);
                },
                [this, id](models::outline::NoOutline const &)
                {
                    _scene._pixelEdgeOutlines.erase(id);
                },
                [this, id](models::outline::PixelEdge const & pixelEdge)
                {
                    _scene._pixelEdgeOutlines[id] = pixelEdge;
                },
            }, effectiveOutline);
        }

        if (invalidatedAny(mask & kParentTransformChanged))
        {
            assert(_mesh);
            _worldAabb = _mesh->aabb();
            _worldAabb.transform(p && p->hasGlobalTransform() ? p->_globalTransform
                                                              : glm::mat4(1));
            _spatialHandler.update(_worldAabb);
        }

        // don't need anything for kEnableOpb (every frame is simply reread)

        Object::revalidate(mask);
    }

    void SceneImpl::DirectionalLightLeaf::revalidate(Mask mask)
    {
        if (invalidatedAny(mask & kColor))
        {
            // a value has changed (probably a new epoch started)
            Lerpable::update(_scene._epochNumber, model());
            _scene.invalidateDeferred(this, kActiveLerp);
        }

        if (invalidatedAny(mask & kActiveLerp) &&
            lerp(_scene._lerpWeight, _scene._epochNumber))
        {
            _scene.invalidateDeferred(this, kActiveLerp);
        }

        Object::revalidate(mask);
    }

    SceneImpl::PointLightLeaf::PointLightLeaf(std::string name,
                                              ModelType const & model,
                                              std::shared_ptr<Node> const & parent,
                                              SceneImpl & scene)
        : LerpableLeaf(std::move(name), model, parent, scene)
        , _localAabb(localAabb())
        , _spatialHandler(*scene._spatialIndex, this, kPointLightLayer)
    {}

    // TODO: possible bug, localAabb (and therefore worldAabb) won't be lerped
    utils::Aabb SceneImpl::PointLightLeaf::localAabb() const
    {
        float const maxRadius = current().maxRadius().value_or(100000.0f);
        return utils::Aabb(-maxRadius, -maxRadius, -maxRadius,
                            maxRadius,  maxRadius,  maxRadius);
    }

    void SceneImpl::PointLightLeaf::revalidate(Mask mask)
    {
        bool forceWorldAabbRecalc = false;

        if (invalidatedAny(mask & (kColor | kAttenuation)))
        {
            // a value has changed (probably a new epoch started)
            Lerpable::update(_scene._epochNumber, model());
            _scene.invalidateDeferred(this, kActiveLerp);

            _localAabb = localAabb();
            forceWorldAabbRecalc = true;
        }

        if (invalidatedAny(mask & kParentTransformChanged) ||
            forceWorldAabbRecalc)
        {
            auto p = _parent.lock();
            _worldAabb = _localAabb;
            _worldAabb.transform(p && p->hasGlobalTransform() ? p->_globalTransform
                                                              : glm::mat4(1));
            _spatialHandler.update(_worldAabb);
        }

        if (invalidatedAny(mask & kActiveLerp) &&
            lerp(_scene._lerpWeight, _scene._epochNumber))
        {
            _scene.invalidateDeferred(this, kActiveLerp);
        }

        Object::revalidate(mask);
    }

    void SceneImpl::PerspectiveCameraLeaf::revalidate(Mask mask)
    {
        if (invalidatedAny(mask & (kYFov /*| kZNear | kZFar | kAspectRatio*/)))
        {
            // a value has changed (probably a new epoch started)
            Lerpable::update(_scene._epochNumber, model());
            _scene.invalidateDeferred(this, kActiveLerp);
        }

        if (invalidatedAny(mask & kActiveLerp) &&
            lerp(_scene._lerpWeight, _scene._epochNumber))
        {
            _scene.invalidateDeferred(this, kActiveLerp);
        }

        Object::revalidate(mask);
    }

    void SceneImpl::PerspectiveCameraLeaf::activate()
    {
        _scene.setActiveCamera(*this);
    }

    void SceneImpl::OrthographicCameraLeaf::revalidate(Mask mask)
    {
        if (invalidatedAny(mask & (kXMag | kYMag /* | kZNear | kZFar*/)))
        {
            // a value has changed (probably a new epoch started)
            Lerpable::update(_scene._epochNumber, model());
            _scene.invalidateDeferred(this, kActiveLerp);
        }

        if (invalidatedAny(mask & kActiveLerp) &&
            lerp(_scene._lerpWeight, _scene._epochNumber))
        {
            _scene.invalidateDeferred(this, kActiveLerp);
        }

        Object::revalidate(mask);
    }

    void SceneImpl::OrthographicCameraLeaf::activate()
    {
        _scene.setActiveCamera(*this);
    }

    SceneImpl::BillboardLeaf::BillboardLeaf(std::string name,
                                            models::Billboard model,
                                            std::shared_ptr<Node> const & parent,
                                            std::shared_ptr<rasterizer::Billboard> const & billboard,
                                            SceneImpl & scene)
        : Leaf(std::move(name), std::move(model), parent, scene)
        , _billboard(billboard)
        , _opbId(scene._enableOpb // OpbId is always allocated (even if disabled in a model)
                    ? std::make_unique<SceneImpl::OpbIdHolder>(scene)
                    : std::unique_ptr<SceneImpl::OpbIdHolder>())
        , _zOrder(model._zOrder)
        , _spatialHandler(*scene._spatialIndex, this, kBillboardLayer)
    {}

    SceneImpl::OpbId SceneImpl::BillboardLeaf::opbId() const
    {
        return _scene._enableOpb && enableOpb() && _opbId ? _opbId->id() : 0;
    }

    void SceneImpl::BillboardLeaf::revalidate(Mask mask)
    {
        auto p = _parent.lock();

        if (OpbId const id = opbId();
            invalidatedAny(mask & kOutline) && id != 0)
        {
            models::Outline const & effectiveOutline =
                p && std::holds_alternative<std::monostate>(outline()) ? p->_effectiveOutline
                                                                       : outline();
            std::visit(utils::Overloaded
            {
                [this, id](std::monostate const &)
                {
                    _scene._pixelEdgeOutlines.erase(id);
                },
                [this, id](models::outline::NoOutline const &)
                {
                    _scene._pixelEdgeOutlines.erase(id);
                },
                [this, id](models::outline::PixelEdge const & pixelEdge)
                {
                    _scene._pixelEdgeOutlines[id] = pixelEdge;
                },
            }, effectiveOutline);
        }

        if (invalidatedAny(mask & kParentTransformChanged))
        {
            assert(_billboard);
            _worldAabb = _scene._rasterizer.billboards().aabb(*_billboard);
            _worldAabb.transform(p && p->hasGlobalTransform() ? p->_globalTransform
                                                              : glm::mat4(1));
            _spatialHandler.update(_worldAabb);
        }

        // don't need anything for kEnableOpb (every frame is simply reread)

        Object::revalidate(mask);
    }

    // SceneImpl::Node //

    scene::Node::Sptr SceneImpl::Node::make(std::string const & name, models::Node model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        Node::Sptr node = std::make_shared<Node>(name, std::move(model), shared_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, node);
        MINIRE_INVARIANT(inserted, "failed to insert \"{}\" into \"{}\"", name, this->name());
        return node;
    }

    scene::Mesh::Sptr SceneImpl::Node::make(std::string const & name, models::Mesh modelSrc)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto mesh = _scene._rasterizer.meshes().getMesh(modelSrc._source,
                                                        modelSrc._defaultMaterial);
        assert(mesh);

        auto meshLeaf = std::make_shared<MeshLeaf>(name, std::move(modelSrc),
                                                   shared_from_this(), mesh, _scene);
        models::Mesh const & modelRef = meshLeaf->model(); // NOTE: "model" was moved-out !!

        {
            auto [_, inserted] = _children.emplace(name, meshLeaf);
            MINIRE_INVARIANT(inserted, "failed to insert \"{}\" into \"{}\"", name, this->name());
        }

        if (meshLeaf->_opbId)
        {
            OpbId const opbId = meshLeaf->_opbId->id();
            assert(opbId != 0);
            _scene._opbIdToSceneItem.resize(std::max<size_t>(_scene._opbIdToSceneItem.size(), opbId + 1));
            MINIRE_INVARIANT(std::holds_alternative<std::monostate>(_scene._opbIdToSceneItem[opbId]),
                             "failed to store OPB ID ({}) of \"{}\"", opbId, name);
            _scene._opbIdToSceneItem[opbId] = meshLeaf;
        }

        if (modelRef._skin)
        {
            models::Mesh::Skin::Bones const & bones = modelRef._skin->_bones;
            meshLeaf->_skinBones.reserve(bones.size());

            MINIRE_INVARIANT(bones.size() <= rasterizer::Constants::kMaxBones,
                             "a mesh {} contains too many bones: {}, limit is {}",
                             name, bones.size(), rasterizer::Constants::kMaxBones);

            for(models::Mesh::Skin::Bone const & boneModel : bones)
            {
                auto node = nodeFromPointer(boneModel._jointNode);
                MINIRE_INVARIANT(node, "a skin bone is null pointer: {}", boneModel._jointNode);

                meshLeaf->_skinBones.emplace_back(MeshLeaf::SkinBone
                {
                    ._inverseBindMatrix = boneModel._inverseBindMatrix,
                    ._node = node,
                });
            }

            meshLeaf->_skinOrigin = modelRef._skin->_origin ? nodeFromPointer(*modelRef._skin->_origin)
                                                         : Node::Wptr();
        }

        meshLeaf->invalidate();
        return meshLeaf;
    }

    scene::DirectionalLight::Sptr SceneImpl::Node::make(std::string const & name,
                                                        models::DirectionalLight model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto directionalLightLeaf = std::make_shared<DirectionalLightLeaf>(name, std::move(model),
                                                                           shared_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, directionalLightLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());
        _scene._directionalLightLeaves.push_back(directionalLightLeaf);
        directionalLightLeaf->invalidate();
        return directionalLightLeaf;
    }

    scene::PointLight::Sptr SceneImpl::Node::make(std::string const & name,
                                                  models::PointLight model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto pointLightLeaf = std::make_shared<PointLightLeaf>(name, std::move(model),
                                                               shared_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, pointLightLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());
        pointLightLeaf->invalidate();
        return pointLightLeaf;
    }

    scene::PerspectiveCamera::Sptr SceneImpl::Node::make(std::string const & name,
                                                         models::PerspectiveCamera model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto perspectiveCameraLeaf = std::make_shared<PerspectiveCameraLeaf>(name, std::move(model),
                                                                             shared_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, perspectiveCameraLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());
        perspectiveCameraLeaf->invalidate();
        return perspectiveCameraLeaf;
    }

    scene::OrthographicCamera::Sptr SceneImpl::Node::make(std::string const & name,
                                                          models::OrthographicCamera model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto orthographicCameraLeaf = std::make_shared<OrthographicCameraLeaf>(name, std::move(model),
                                                                               shared_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, orthographicCameraLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());
        orthographicCameraLeaf->invalidate();
        return orthographicCameraLeaf;
    }

    scene::Billboard::Sptr SceneImpl::Node::make(std::string const & name,
                                                 models::Billboard model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto billboard = _scene._rasterizer.billboards().create(model);
        assert(billboard);

        auto billboardLeaf = std::make_shared<BillboardLeaf>(name, std::move(model),
                                                             shared_from_this(), billboard, _scene);
        auto [_, inserted] = _children.emplace(name, billboardLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());

        if (billboardLeaf->_opbId)
        {
            OpbId const opbId = billboardLeaf->_opbId->id();
            assert(opbId != 0);
            _scene._opbIdToSceneItem.resize(std::max<size_t>(_scene._opbIdToSceneItem.size(), opbId + 1));
            MINIRE_INVARIANT(std::holds_alternative<std::monostate>(_scene._opbIdToSceneItem[opbId]),
                             "failed to store OPB ID ({}) of \"{}\"", opbId, name);
            _scene._opbIdToSceneItem[opbId] = billboardLeaf;
        }

        billboardLeaf->invalidate();
        return billboardLeaf;
    }

    void SceneImpl::Node::makeFromSource(content::Path const & source,
                                         content::Manager & contentManager,
                                         bool visible)
    {
        instantiateGltf(*this, source, contentManager, visible);
    }

    SceneImpl::AnimationTracksSptr
    SceneImpl::Node::instantiateTracks(models::AnimationTracks const & animationTracks) const
    {
        AnimationTracksSptr result = std::make_shared<AnimationTracks>();
        result->reserve(animationTracks.size());
        for(auto const & [target, keyframeAnimation] : animationTracks)
        {
            auto const & targetSptr = nodeFromPointer(target);
            MINIRE_INVARIANT(targetSptr, "no valid animation target: {}", target);
            result->emplace_back(AnimationTrack
            {
                ._target = targetSptr,
                ._animation = scene::makeKeyframeAnimation(keyframeAnimation),
            });
        }
        return result;
    }

    void SceneImpl::Node::makeAnimationSet(models::AnimationSet animationSet) // TODO: const ref?
    {
        // drop any current active animation
        _playbackStack.clear();

        // transform animation set from abstract (model) into a concrete one
        AnimationSet newAnimationSet;
        newAnimationSet.reserve(animationSet.size());
        for(auto const & [animationId, animationTracks] : animationSet)
        {
            newAnimationSet.emplace(animationId,
                                    instantiateTracks(animationTracks));
        }
        _animationSet = std::move(newAnimationSet);
    }

    // TODO: when parent is changed, some animation may stil refer to moved nodes,
    //       it will work, but ill-logic.
    void SceneImpl::Node::setParent(scene::Node::Sptr const & newParentIface)
    {
        _scene.setParent(*this, newParentIface);
    }

    models::ScenePath SceneImpl::Node::absPath() const
    {
        models::ScenePath result;
        result.reserve(10); // hmmm
        for (auto node = shared_from_this(); node; node = node->_parent.lock())
        {
            result.push_back(node->name());
        }
        std::ranges::reverse(result);
        return result;
    }

    void SceneImpl::Node::erase(models::ScenePath const & path)
    {
        if (auto it = findIterator(path); !it.empty())
        {
            it.erase();
        }
    }

    void SceneImpl::Node::clear()
    {
        _children.clear();
    }

    std::vector<scene::SceneItem> SceneImpl::Node::children() const
    {
        std::vector<scene::SceneItem> result;
        result.reserve(_children.size());
        for (auto const & [_, child] : _children)
        {
            result.emplace_back(
                std::visit([](auto const & child) -> scene::SceneItem { return child; }, child));
        }
        return result;
    }

    scene::SceneItem
    SceneImpl::Node::find(models::ScenePath const & path) const
    {
        if (auto it = findIterator(path); !it.empty())
        {
            return std::visit(
                [](auto const & child) -> scene::SceneItem { return child; },
                it.item());
        }
        return std::monostate();
    }

    SceneImpl::Node::Node(std::string name,
                          Object::ModelType && model,
                          Sptr const & parent,
                          SceneImpl & scene)
        : scene::Node(std::move(name), std::move(model), kNoFlags)
        , _scene(scene)
        , _depth(parent ? (parent->_depth != kNoDepth ? parent->_depth + 1 : kNoDepth) : 0) // NOTE: have to be 0 for the "_root"-case
        , _localTransform(std::as_const(*this).origin())
        , _globalTransform(glm::mat4(1))
        , _parent(parent)
        , _playbackStack(*this)
        , _effectiveOutline(std::monostate())
        , _effectiveVisible(true)
    {
        // NOTE: calling it with explicit "Node::"-qualifier to ensure
        //       safety of virtual call from a ctor
        Node::invalidate(kDirtyTransform);
    }

    SceneImpl::Node::~Node()
    {
        if (kNoDepth != _depth)
        {
            _scene.changeNodeLevel(this, _depth, kNoDepth);
        }
    }

    bool SceneImpl::Node::lerp(float weight, size_t epochNumber)
    {
        return _localTransform.lerp(weight, epochNumber);
    }

    // TODO: cover by tests
    SceneImpl::Node::ItemIterator
    SceneImpl::Node::findIterator(models::ScenePath const & path) const
    {
        Node const * node = this;
        ChildrenMap::const_iterator it = node->_children.cend();
        size_t index = 0;
        for(; index < path.size(); ++index)
        {
            assert(node);
            it = node->_children.find(path[index]);
            if (it == node->_children.cend())
                break;
            bool const isLast = (index == path.size() - 1);
            if (Node::Sptr const * asNode = std::get_if<Node::Sptr>(&it->second);
                asNode && !isLast)
            {
                assert(*asNode);
                node = asNode->get();
                it = node->_children.cend();
            }
        }

        if (index != path.size())
        {
            assert(node);
            it = node->_children.cend();
        }

        assert(node);
        return ItemIterator{._parent = node, ._iterator = it};
    }

    template<typename T>
    typename T::Sptr const & SceneImpl::Node::findInternal(models::ScenePath const & path) const
    {
        // find an interator
        auto it = findIterator(path);
        MINIRE_INVARIANT(!it.empty(), "no such item \"{}\" inside \"{}\"",
                         path, name());

        // get a value
        typename T::Sptr const * fetched = std::get_if<typename T::Sptr>(&it.item());
        MINIRE_INVARIANT(fetched, "element at path \"{}\" is not {}",
                         path, utils::demangle<T>());
        return *fetched;
    }

    SceneImpl::Node::Sptr
    SceneImpl::Node::nodeFromPointer(models::NodePointer const & nodePointer) const
    {
        return std::visit(utils::Overloaded
        {
            [this](models::ScenePath const & p) { return findInternal<Node>(p); },
            [](scene::Node::Sptr const & p) { return std::static_pointer_cast<Node>(p); },
        }, nodePointer);
    }

    bool SceneImpl::Node::advanceAnimation()
    {
        SceneImpl::ActiveAnimation * activeAnimation = _playbackStack.activeAnimation();
        if (!activeAnimation) return false;

        if (activeAnimation->_paused) return true;

        // advance all sequencers (that aren't done)
        for(auto sequencer : activeAnimation->_uniqueSequencers)
        {
            assert(sequencer);
            if (!sequencer->isDone())
            {
                sequencer->advance(_scene._frameTime);
            }
        }

        // update transformation
        assert(activeAnimation->_animationTracks);
        assert(activeAnimation->_animationSequencers.size() == activeAnimation->_animationTracks->size());
        for(size_t i = 0; i < activeAnimation->_animationTracks->size(); ++i)
        {
            AnimationTrack & animationTrack = (*activeAnimation->_animationTracks)[i];
            ActiveAnimation::SequencerSet const & sequencerSet = activeAnimation->_animationSequencers[i];

            if (Node::Sptr const & targetNode = animationTrack._target.lock();
                targetNode)
            {
                assert(animationTrack._animation);
                scene::KeyframeAnimation const & anim = *animationTrack._animation;
                models::Transform current = targetNode->_localTransform.current();

                bool hasTrack = false;
                if (anim._translation && sequencerSet._translation &&
                    !sequencerSet._translation->isDone())
                {
                    current._translation = sequencerSet._translation->current(*anim._translation);
                    hasTrack |= true;
                }

                if (anim._rotation && sequencerSet._rotation &&
                    !sequencerSet._rotation->isDone())
                {
                    current._rotation = sequencerSet._rotation->current(*anim._rotation);
                    hasTrack |= true;
                }

                if (anim._scale && sequencerSet._scale &&
                    !sequencerSet._scale->isDone())
                {
                    current._scale = sequencerSet._scale->current(*anim._scale);
                    hasTrack |= true;
                }

                if (hasTrack)
                {
                    // NOTE: don't perform actual lerp for animable targets
                    bool const changed = current != targetNode->_localTransform.current();
                    targetNode->_localTransform.setCurrent(_scene._epochNumber, current);
                    if (changed) _scene.activateNode(targetNode.get(), kDirtyTransform);
                }
            }
        }

        // clean up finished playbacks
        // TODO: finished animations may overlap with a next one,
        //       so the next one should be advanced in before
        while (0 != _playbackStack.size() &&
               _playbackStack.top()->status() == scene::Node::PlaybackController::Status::kFinished)
        {
            _playbackStack.pop();
        }

        return 0 != _playbackStack.size();
    }

    void SceneImpl::Node::revalidateAnimation()
    {
        assert(invalidatedAll(kActiveAnimation));
        revalidate(kActiveAnimation);
        // NOTE: advanceAnimation() will kDirtyTransform if needed
        if (advanceAnimation())
        {
            // animation can be advanced again during the next advance()
            _scene.invalidateDeferred(this, kActiveAnimation);
        }
    }

    void SceneImpl::Node::revalidateOrigin()
    {
        assert(invalidatedAll(kOrigin));
        revalidate(kOrigin);
         // std::as_const to ensure it won't cause invalidation of origin()
        _localTransform.update(_scene._epochNumber,
                               std::as_const(*this).origin());
        _scene.activateNode(this, kActiveLerp);
    }

    void SceneImpl::Node::revalidateLerp()
    {
        assert(invalidatedAll(kActiveLerp));
        revalidate(kActiveLerp);
        if (lerp(_scene._lerpWeight, _scene._epochNumber))
        {
            _scene.activateNode(this, kDirtyTransform);
            _scene.invalidateDeferred(this, kActiveLerp);
        }
    }

    void SceneImpl::Node::revalidateTransform()
    {
        static const glm::mat4 kIdentityMatrix(glm::identity<glm::mat4>());
        static const glm::vec4 kGlobalOrigin(0, 0, 0, 1);

        assert(invalidatedAll(kDirtyTransform));
        revalidate(kDirtyTransform);

        // update local transform
        // TODO: localTransform.matrix() is pretty expensive, should avoid it
        //       if only parent's transform has been changed
        models::Transform const & localTransform = _localTransform.current();
        _localTransformMatrix = localTransform.matrix();

        // recalc global transform
        auto parent = _parent.lock();
        assert(!parent || parent->hasGlobalTransform());
        glm::mat4 const & parentGlobalTransform = parent ? parent->_globalTransform
                                                         : kIdentityMatrix;
        if (glm::mat4 globalTransform = parentGlobalTransform * _localTransformMatrix;
            _globalTransform != globalTransform)
        {
            _globalTransform = std::move(globalTransform);
            _globalPosition = _globalTransform * kGlobalOrigin; // will drop "w"

            // ensure that children's matrices will be recalculated
            invalidateChildren<MeshLeaf::Sptr>(MeshLeaf::kParentTransformChanged);
            invalidateChildren<PointLightLeaf::Sptr>(PointLightLeaf::kParentTransformChanged);
            invalidateChildren<BillboardLeaf::Sptr>(BillboardLeaf::kParentTransformChanged);
            invalidateChildren<Node::Sptr>(Node::kDirtyTransform);
        }
    }

    void SceneImpl::Node::revalidateOutline()
    {
        assert(invalidatedAll(kOutline));
        revalidate(kOutline);

        auto parent = _parent.lock();
        models::Outline const & newOutline =
            std::holds_alternative<std::monostate>(outline()) && parent
                ? parent->_effectiveOutline // the node has no explicitly-set outline and the Node has a parent, inherit it from a parent
                : outline();                // the node has no parent or it's outline is explicitly set
        if (_effectiveOutline != newOutline)
        {
            _effectiveOutline = newOutline;
            invalidateChildren<Node::Sptr>(Node::kOutline);
            invalidateChildren<MeshLeaf::Sptr>(MeshLeaf::kOutline);
            invalidateChildren<BillboardLeaf::Sptr>(BillboardLeaf::kOutline);
        }
    }

    void SceneImpl::Node::revalidateVisiblity()
    {
        assert(invalidatedAll(kVisible));
        revalidate(kVisible);

        auto parent = _parent.lock();
        bool const oldEffectiveVisible = _effectiveVisible;
        _effectiveVisible = visible() && (parent ? parent->_effectiveVisible : true);
        if (oldEffectiveVisible != _effectiveVisible)
        {
            invalidateChildren<Node::Sptr>(Node::kVisible);
        }
    }

    void SceneImpl::Node::invalidate(Mask mask)
    {
        // NOTE: SceneImpl::activateNode() will call Object::invalidate of this Node,
        //       so, don't need to re-call it here.
        _scene.activateNode(this, mask);
    }

    // NOTE: won't propagate upwards or downwards, only updates flags of direct children!
    template<typename T>
    void SceneImpl::Node::invalidateChildren(Mask mask)
    {
        for(auto const & [_, child] : _children)
        {
            if (T const * item = std::get_if<T>(&child); item)
            {
                assert(*item);
                (*item)->invalidate(mask);
            }
        }
    }

    // ActiveAnimation //

    SceneImpl::ActiveAnimation::ActiveAnimation(AnimationTracksSptr const & animationTracks,
                                                size_t const repeats,
                                                float const speedScale)
        : _animationTracks(animationTracks)
    {
        auto getOrMakeSequencer = [this, repeats, speedScale]
                                  (Sequencer::Timeline const & timeline)
        {
            auto it = std::find_if(_uniqueSequencers.cbegin(),
                                   _uniqueSequencers.cend(),
                                   [&timeline](Sequencer::Sptr const & i)
                                   {
                                        assert(i);
                                        return i->timeline() == timeline;
                                   });
            if (it != _uniqueSequencers.cend())
            {
                return *it;
            }
            auto newSequencer = std::make_shared<Sequencer>(timeline, repeats, speedScale);
            _uniqueSequencers.emplace_back(newSequencer);
            return newSequencer;
        };

        assert(_animationTracks);
        _animationSequencers.reserve(_animationTracks->size());
        for(AnimationTrack const & animationTrack : *_animationTracks)
        {
            assert(animationTrack._animation);
            scene::KeyframeAnimation const & anim = *animationTrack._animation;

            _animationSequencers.emplace_back(SequencerSet{});
            SequencerSet & sequencerSet = _animationSequencers.back();

            if (anim._translation)
            {
                sequencerSet._translation = getOrMakeSequencer(anim._translation->inputs());
            }
            if (anim._rotation)
            {
                sequencerSet._rotation = getOrMakeSequencer(anim._rotation->inputs());
            }
            if (anim._scale)
            {
                sequencerSet._scale = getOrMakeSequencer(anim._scale->inputs());
            }
        }

        assert(_animationTracks->size() == _animationSequencers.size());
    }

    // PlaybackController //

    scene::Node::PlaybackController::Status SceneImpl::ActiveAnimation::status() const
    {
        if (_paused) return scene::Node::PlaybackController::Status::kPaused;

        bool hasNonDoneSequencer = std::ranges::any_of(
            _uniqueSequencers,
            [](ActiveAnimation::Sequencer::Sptr const & sequencer)
            {
                assert(sequencer);
                return !sequencer->isDone();
            });

        return hasNonDoneSequencer ? scene::Node::PlaybackController::Status::kActive
                                   : scene::Node::PlaybackController::Status::kFinished;
    }

    void SceneImpl::ActiveAnimation::setPaused(bool paused)
    {
        _paused = paused;
    }

    // PlaybackStackImpl //

    SceneImpl::PlaybackStackImpl::PlaybackStackImpl(Node & node)
        : _node(node)
    {
        _activeAnimations.reserve(16);
    }

    void SceneImpl::PlaybackStackImpl::push(models::AnimationId const & animationId,
                                            size_t repeats,
                                            float speedScale)
    {
        auto it = _node._animationSet.find(animationId);
        MINIRE_INVARIANT(it != _node._animationSet.cend(),
                         "no such animation: {} in {}", animationId, _node.name());

        assert(it->second);
        auto newAnimation = std::make_unique<SceneImpl::ActiveAnimation>(
            it->second, repeats, speedScale);
        _activeAnimations.emplace_back(std::move(newAnimation));
        _node.invalidate(SceneImpl::Node::kActiveAnimation);
    }

    void SceneImpl::PlaybackStackImpl::push(models::AnimationTracks animationTracks,
                                            size_t repeats,
                                            float speedScale)
    {
        auto newAnimation = std::make_unique<SceneImpl::ActiveAnimation>(
            _node.instantiateTracks(animationTracks), repeats, speedScale);
        _activeAnimations.emplace_back(std::move(newAnimation));
        _node.invalidate(SceneImpl::Node::kActiveAnimation);
    }

    void SceneImpl::PlaybackStackImpl::pop()
    {
        if (!_activeAnimations.empty())
        {
            _activeAnimations.pop_back();
        }
    }

    scene::Node::PlaybackController * SceneImpl::PlaybackStackImpl::top() const
    {
        return activeAnimation();
    }

    scene::Node::PlaybackController * SceneImpl::PlaybackStackImpl::bottom() const
    {
        return !_activeAnimations.empty() ? _activeAnimations.front().get() : nullptr;
    }

    SceneImpl::ActiveAnimation * SceneImpl::PlaybackStackImpl::activeAnimation() const
    {
        return !_activeAnimations.empty() ? _activeAnimations.back().get() : nullptr;
    }

    // Scene //

    SceneImpl::OpbId SceneImpl::allocateOpbId()
    {
        if (_vacantOpbIds.empty())
        {
            MINIRE_INVARIANT(_maxOpbId != std::numeric_limits<OpbId>::max(),
                             "too many object on a scene to select: {}", _maxOpbId);
            return _maxOpbId++;
        }

        OpbId result = _vacantOpbIds.back();
        _vacantOpbIds.pop_back();

        assert(!_pixelEdgeOutlines.contains(result));

        return result;
    }

    void SceneImpl::releaseOpbId(OpbId opbId)
    {
        assert(opbId != 0);

        assert(opbId < _opbIdToSceneItem.size());
        _opbIdToSceneItem[opbId] = std::monostate();

        _pixelEdgeOutlines.erase(opbId);
        _vacantOpbIds.emplace_back(opbId);
    }

    SceneImpl::SceneImpl(Rasterizer & rasterizer)
        : _rasterizer(rasterizer)
        , _spatialIndex(std::make_unique<scene::spatial_index::BruteForce>())
        , _enableOpb(true)  // TODO: make OPB optional (as an attachable unit for The NextGenPipeline (NGP))
                            // TODO: MASS should be disable when OPB is used
    {
        reset();
    }

    scene::Node & SceneImpl::root() const
    {
        assert(_root);
        return *_root;
    }

    void SceneImpl::setActiveCamera(PerspectiveCameraLeaf & camera)
    {
        _activeCamera = ActiveCamera(camera.weak_from_this());
        _viewpoint.setCamera(camera.current());
    }

    void SceneImpl::setActiveCamera(OrthographicCameraLeaf & camera)
    {
        _activeCamera = ActiveCamera(camera.weak_from_this());
        _viewpoint.setCamera(camera.current());
    }

    void SceneImpl::setActiveCamera(models::ScenePath const & path)
    {
        if (path.empty())
        {
            _activeCamera = std::monostate();
            _viewpoint.unsetCamera();
        }
        else
        {
            assert(_root);
            Node::ItemIterator it = _root->findIterator(path);
            MINIRE_INVARIANT(!it.empty(), "no such camera: {}", path);
            std::visit(utils::Overloaded
            {
                [this](PerspectiveCameraLeaf::Sptr const & camera)
                {
                    assert(camera);
                    camera->activate();
                },

                [this](OrthographicCameraLeaf::Sptr const & camera)
                {
                    assert(camera);
                    camera->activate();
                },

                [&path](auto && item)
                {
                    using T = std::decay_t<decltype(item)>;
                    MINIRE_THROW("got {} instead of a camera: {}",
                                 utils::demangle<T>(), path);
                }
            }, it.item());
        }
    }

    void SceneImpl::reset()
    {
        _pendedActivations.reset();

        _meshCullBuffer.clear();
        _billboardCullBuffer.clear();
        _directionalLightLeaves.clear();
        _pointLightCullBuffer.clear();
        _billboardWideCullBuffer.clear();

        _root = std::make_shared<Node>("(the root)",
            models::Node{models::Transform{}},
            Node::Sptr(), *this);
    }

    void SceneImpl::setupSpatialIndex(scene::SpatialIndex::Uptr && spatialIndex)
    {
        reset();

        if (spatialIndex)
        {
            _spatialIndex = std::move(spatialIndex);
        }
        else
        {
            _spatialIndex = std::make_unique<scene::spatial_index::BruteForce>();
        }

        // SpatialIndex must always persist
        assert(_spatialIndex);
    }

    scene::SceneItem SceneImpl::fetchSceneItem(size_t const x, size_t const y) const
    {
        if (!_enableOpb)
            return std::monostate();

        return fetchSceneItem(_rasterizer.fetchMeshId(x, y));
    }

    scene::SceneItem SceneImpl::fetchHotSceneItem() const
    {
        if (!_enableOpb)
            return std::monostate();

        return fetchSceneItem(_rasterizer.fetchHotMeshId());
    }

    scene::SceneItem SceneImpl::fetchSceneItem(OpbId const opbId) const
    {
        if (!_enableOpb)
            return std::monostate();

        if (0 == opbId)
            return std::monostate();

        if (opbId >= _opbIdToSceneItem.size())
            return std::monostate();

        return std::visit(utils::Overloaded
        {
            [](std::monostate const &) -> scene::SceneItem { return std::monostate(); },
            [](MeshLeaf::Wptr const & item) -> scene::SceneItem { return item.lock(); },
            [](BillboardLeaf::Wptr const & item) -> scene::SceneItem { return item.lock(); },
        }, _opbIdToSceneItem[opbId]);
    }

    void SceneImpl::setViewport(size_t weight, size_t height)
    {
        _viewpoint.setViewport(weight, height);
    }

    void SceneImpl::actualizeViewpoint()
    {
        std::visit(utils::Overloaded
        {
            [](std::monostate) {},
            [this](auto const & wcamera)
            {
                if (auto const & camera = wcamera.lock())
                {
                    Node::Sptr parent = camera->_parent.lock();
                    MINIRE_INVARIANT(parent, "an active camera has no parent");
                    MINIRE_INVARIANT(parent->hasGlobalTransform(),
                                     "an active camera's node has no global transform");
                    // TODO: don't update if transform didn't change
                    //       (to avoid expensive change checks inside setTransform)
                    _viewpoint.setTransform(parent->_globalTransform,
                                            parent->_globalPosition);

                    // TODO: don't update unless camera or its parameters changed
                    //       (to avoid expensive change checks inside setCamera)
                    _viewpoint.setCamera(camera->current());
                }
                else
                {
                    _activeCamera = std::monostate();
                    _viewpoint.unsetCamera();
                }
            },
        }, _activeCamera);
    }

    template<typename Target, typename Callback>
    void SceneImpl::ActivationLevel::flush(Target ActivationLevel::* listPtr,
                                           uint32_t Node::* indexPtr,
                                           Callback callback)
    {
        auto & container = this->*listPtr;
        size_t const size = container.size();
        for(size_t i = 0; i < size; ++i)
        {
            Node * node = container[i];
            assert(node);
            callback(*node);
            node->*indexPtr = Node::kNoIndex;
        }
        assert(size == container.size());
        container.clear();
    }

    /**
     * - objects that were set directly (via setOrigin() and such),
     *   will be lerped (where applicable). Such objects are affected by
     *   the Epoch change.
     * - objects that are controlled by an Animation will be,
     *   will be advanced by the Animation (i.e. won't be lerped).
     *   Such objects are only affected by an elapsed frame time.
     *
     * An Epoch consists of one or more Frames.
     * */
    void SceneImpl::advance(size_t const epochNumber,
                            double const epochTime,
                            double const epochDuration,
                            double const frameTime)
    {
        assert(_root);

        // detect epoch start
        assert(epochNumber >= _epochNumber);
        bool const epochStarted = epochNumber != _epochNumber;
        _epochNumber = epochNumber;
        _frameTime = frameTime;

        assert(_deferredNodesInvalidation.empty());
        assert(_deferredLeavesInvalidation.empty());

        // Pass 1. advance animable objects
        _pendedActivations.flush(kActiveAnimationLocator._listPtr,
                                 kActiveAnimationLocator._indexPtr,
                                 [](Node & node) { node.revalidateAnimation(); });

        // Pass 2. advance directly set values for a new Epoch or
        //         perform lerping for continuing Epoch
        if (epochStarted)
        {
            // transfer accumulated models state into scene instances
            _pendedActivations.flush(kDirtyOriginLocator._listPtr,
                                     kDirtyOriginLocator._indexPtr,
                                     [](Node & node) { node.revalidateOrigin(); });
        }

        _lerpWeight = epochDuration != 0 ? epochTime / epochDuration : 1.0;
        assert(_lerpWeight >= 0);
        _pendedActivations.flush(kActiveLerpLocator._listPtr,
                                 kActiveLerpLocator._indexPtr,
                                 [](Node & node) { node.revalidateLerp(); });

        // Pass 3. revalidate effective values
        //         (transforms, outlines, visibility, etc)
        _pendedActivations.flush(kDirtyTransformLocator._listPtr,
                                 kDirtyTransformLocator._indexPtr,
                                 [](Node & node) { node.revalidateTransform(); });
        _pendedActivations.flush(kDirtyOutlineLocator._listPtr,
                                 kDirtyOutlineLocator._indexPtr,
                                 [](Node & node) { node.revalidateOutline(); });
        _pendedActivations.flush(kDirtyVisibleLocator._listPtr,
                                 kDirtyVisibleLocator._indexPtr,
                                 [](Node & node) { node.revalidateVisiblity(); });

        // Leaves pass
        for(size_t i = 0; i < _pendedActivations._leaves.size(); ++i)
        {
            utils::ObjectBase * leaf = _pendedActivations._leaves[i];

            assert(leaf);
            leaf->revalidate();
            leaf->_activationIndex = leaf->kNoIndex;
        }
        _pendedActivations._leaves.clear();

        // Viewport may be changed if Camera's node has been transformed
        actualizeViewpoint();

        // apply deferred invalidations (for the next advance() call)
        for(auto const & [node, mask] : _deferredNodesInvalidation)
        {
            assert(node);
            node->invalidate(mask); // invalidate() MUST NOT alter
                                    // _deferredNodesInvalidation
        }
        _deferredNodesInvalidation.clear();

        for(auto const & [leaf, mask] : _deferredLeavesInvalidation)
        {
            assert(leaf);
            leaf->invalidate(mask); // invalidate() MUST NOT alter
                                    // _deferredLeavesInvalidation
        }
        _deferredLeavesInvalidation.clear();

#       ifndef NDEBUG
        // for debug-only: ensure that no nodes has been left invalidated
        std::vector<Node const *> queue;
        while(!queue.empty())
        {
            Node const * node = queue.back();
            queue.pop_back();

            // NOTE: Node::kActiveAnimation may be stored between iterations
            assert(node);
            assert(node->invalidatedAny(Node::kActiveAnimation) ||
                   !node->invalidated());

            for(auto & [_, child] : node->_children)
            {
                std::visit(utils::Overloaded
                {
                    [&queue](Node::Sptr const & childNode)
                    {
                        assert(childNode);
                        queue.emplace_back(childNode.get());
                    },
                    [](auto const & leaf) { assert(leaf && !leaf->invalidated()); },
                }, child);
            }
        }
#       endif
    }

    void SceneImpl::invalidateDeferred(Node * node, Node::Mask mask)
    {
        _deferredNodesInvalidation.emplace_back(node, mask);
    }

    template<typename Derived, typename ObjectType>
    void SceneImpl::invalidateDeferred(Leaf<Derived, ObjectType> * leaf,
                                       typename ObjectType::Mask mask)
    {
        _deferredLeavesInvalidation.emplace_back(leaf, mask);
    }

    template<typename Callback>
    void SceneImpl::activationMappings(Callback callback)
    {
        callback(kActiveAnimationLocator);
        callback(kDirtyOriginLocator);
        callback(kActiveLerpLocator);
        callback(kDirtyTransformLocator);
        callback(kDirtyOutlineLocator);
        callback(kDirtyVisibleLocator);
    }

    template<typename Target>
    void SceneImpl::ActivationLevel::activate(Target ActivationLevel::* listPtr,
                                              uint32_t Node::* indexPtr,
                                              Node * node, Node::Mask const mask,
                                              bool const force)
    {
        assert(node);
        assert(std::has_single_bit(mask));
        if (node->*indexPtr == Node::kNoIndex &&
            (force || !node->invalidatedAny(mask)))
        {
            auto & container = this->*listPtr;
            container.push_back(node);
            node->*indexPtr = container.size() - 1;
            assert(Node::kNoIndex != node->*indexPtr);
            node->Object::invalidate(mask);
        }
    }

    void SceneImpl::activateNode(Node * node, Node::Mask mask)
    {
        assert(node);
        if (size_t const depth = node->_depth; depth != kNoDepth)
        {
             // prepare a level to be filled
            _pendedActivations._nodes.resize(std::max(_pendedActivations._nodes.size(), depth + 1));
            ActivationLevel & activationLevel = _pendedActivations._nodes[depth];

            activationMappings(
                [mask, node, &activationLevel]
                (ActivatedNodeLocator const & locator)
                {
                    if (mask & locator._mask)
                    {
                        activationLevel.activate(locator._listPtr,
                                                 locator._indexPtr,
                                                 node, mask & locator._mask);
                    }
                });
        }
    }

    template<typename Target>
    bool SceneImpl::ActivationLevel::erase(Target ActivationLevel::* listPtr,
                                           uint32_t Node::* indexPtr,
                                           Node * node)
    {
        assert(node);
        if (size_t const index = node->*indexPtr;
            index != Node::kNoIndex)
        {
            auto & container = this->*listPtr;
            assert(index < container.size());

            Node * movedNode = container.back();
            container[index] = movedNode;
            movedNode->*indexPtr = index;

            container.pop_back();
            node->*indexPtr = Node::kNoIndex;

            return true;
        }
        return false;
    }

    void SceneImpl::changeNodeLevel(Node * node, size_t oldDepth, size_t newDepth)
    {
        assert(node);

        // fetch an old level
        assert(oldDepth < _pendedActivations._nodes.size());
        ActivationLevel & oldActivationLevel = _pendedActivations._nodes[oldDepth];

        // maybe prepare a new level
        ActivationLevel * newActivationLevel = nullptr;
        if (kNoDepth != newDepth)
        {
            _pendedActivations._nodes.resize(std::max(_pendedActivations._nodes.size(), newDepth + 1));
            newActivationLevel = &_pendedActivations._nodes[newDepth];
        }

        // perform change operations
        activationMappings(
            [&oldActivationLevel, newActivationLevel, node]
            (ActivatedNodeLocator const & locator)
            {
                if (oldActivationLevel.erase(locator._listPtr, locator._indexPtr, node) && newActivationLevel)
                {
                    newActivationLevel->activate(locator._listPtr, locator._indexPtr, node, locator._mask, true);
                }
            });
    }

    template<typename Derived, typename ObjectType>
    void SceneImpl::activateLeaf(Leaf<Derived, ObjectType> * leaf,
                                 typename ObjectType::Mask mask)
    {
        assert(leaf);
        if (!leaf->invalidated())
        {
            // leaf can be added to the activation list just once by an any flags
            _pendedActivations._leaves.push_back(leaf);
            assert(leaf->_activationIndex == leaf->kNoIndex);
            leaf->_activationIndex = _pendedActivations._leaves.size() - 1;
        }
        leaf->ObjectType::invalidate(mask);
    }

    template<typename Derived, typename ObjectType>
    void SceneImpl::eraseLeaf(Leaf<Derived, ObjectType> * leaf)
    {
        if (size_t const leafIndex = leaf->_activationIndex;
            leafIndex != leaf->kNoIndex)
        {
            assert(leafIndex < _pendedActivations._leaves.size());

            utils::ObjectBase * movedLeaf = _pendedActivations._leaves.back();
            _pendedActivations._leaves[leafIndex] = movedLeaf;

            movedLeaf->_activationIndex = leafIndex;

            _pendedActivations._leaves.pop_back();
            leaf->_activationIndex = leaf->kNoIndex;
        }
    }

    // TODO: shouldn't be a static method?
    material::SkinningVectorSptr
    SceneImpl::makeSkinningVector(MeshLeaf const & mesh) const
    {
        static const glm::mat4 kIdentityMatrix(glm::identity<glm::mat4>());

        if (mesh._skinBones.empty()) return {};

        material::SkinningVectorSptr result =
            std::make_shared<material::SkinningVector>(mesh._skinBones.size());

        size_t offset = 0;
        for(MeshLeaf::SkinBone const & skinBone : mesh._skinBones)
        {
            if (auto const & node = skinBone._node.lock();
                node && node->hasGlobalTransform())
            {
                (*result)[offset++] = glm::mat4(node->_globalTransform * skinBone._inverseBindMatrix);
            }
            else
            {
                // TODO: add more debug info
                (*result)[offset++] = kIdentityMatrix;
                MINIRE_WARNING("skinBone refers to a non-existing Node or "
                               "its's global transform isn't clear");
            }
        }
        assert(offset == result->size());

        return result;
    }

    template<typename ItemType>
    void SceneImpl::setParent(ItemType & item,
                              scene::Node::Sptr const & newParentIface)
    {
        // find a leaf's iterator
        auto oldParent = item._parent.lock();
        MINIRE_INVARIANT(oldParent, "an un-parented scene leaf cannot be re-parented: \"{}\"",
                         item.name());
        auto oldIterator = oldParent->_children.find(item.name());
        assert(oldIterator != oldParent->_children.cend());

        // find a new parent and insert element to a new parent's children
        SceneImpl::Node::Sptr newParent = std::static_pointer_cast<SceneImpl::Node>(newParentIface);
        auto itemHardCopy = oldIterator->second;    // an explicit copy to ensure item's lifetime
                                                    // in case when newParent is nullptr
        if (newParent)
        {
            auto [_, inserted] = newParent->_children.emplace(oldIterator->first, itemHardCopy);
            MINIRE_INVARIANT(inserted, "failed to insert \"{}\" into a new parent node (\"{}\")",
                             item.name(), newParent->name());
        }

        // reset _parent for the element
        item._parent = newParent;

        // erase the element from an old parent, must be safe
        // since itemHardCopy keeps an Sptr reference to item
        oldParent->_children.erase(oldIterator);

        // rebuild tree structure
        if constexpr(std::is_same_v<ItemType, Node>)
        {
            // mark current node invalidated, all children's
            // will be transforms will be invalidated during advance()
            item.invalidate(Node::kDirtyTransform);

            std::vector<Node *> queue{&item};
            while(!queue.empty())
            {
                // fetch the next item
                Node * current = queue.back();
                queue.pop_back();
                assert(current);

                // recalculate current's _depth
                auto parent = current->_parent.lock();
                size_t const oldDepth = current->_depth;
                current->_depth = parent && parent->_depth != kNoDepth ? parent->_depth + 1
                                                                       : kNoDepth;
                if (oldDepth != current->_depth)
                {
                    // rebuild activation lists for current
                    changeNodeLevel(current, oldDepth, current->_depth);

                    // process children
                    for(auto const & [_, child] : current->_children)
                    {
                        std::visit(utils::Overloaded
                        {
                            [&queue](Node::Sptr const & node)
                            {
                                // enqueue Node to be traversed
                                assert(node);
                                queue.push_back(node.get());
                            },
                            [current](auto const & leaf)
                            {
                                // update a leaf in-place
                                assert(leaf);
                                leaf->_depth = current->_depth != kNoDepth ? current->_depth + 1
                                                                           : kNoDepth;
                                using Leaf = std::decay_t<decltype(leaf)>::element_type;
                                static_assert(!std::is_same_v<Leaf, SceneImpl::Node>,
                                              "the visitor is broken!");
                            }
                        }, child);
                    }
                }
            }
        }
        else
        {
            // leaves don't need any traverse
            item._depth = newParent && newParent->_depth != kNoDepth ? newParent->_depth + 1
                                                                     : kNoDepth;
            item.invalidate(ItemType::kParentTransformChanged);
        }
    }
}
