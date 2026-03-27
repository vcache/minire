#include <scene-impl.hpp>

#include <rasterizer.hpp>
#include <rasterizer/constants.hpp>
#include <scene-impl/gltf-instantiator.hpp>
#include <utils/overloaded.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/geometry.hpp>

#include <fmt/ranges.h>

#include <algorithm>
#include <cassert>
#include <vector>

namespace minire
{
    /**
     * PLAN:
     *  - think about sorting (per program/per material/per texture)
     *  - visible/invisible different lists: nodes, sprites, labels
     *  - think culling index abstraction
     * */

    // TODO: store/calculate absolute path for nodes/leaves (and use them for logging)

    // SceneImpl::Leaf //

    template<typename Derived, typename ObjectType>
    void SceneImpl::Leaf<Derived, ObjectType>::setParent(scene::Node::Sptr const & newParentIface)
    {
        SceneImpl::setParent(*this, newParentIface);
    }

    template<typename Derived, typename ObjectType>
    void SceneImpl::Leaf<Derived, ObjectType>::propagate(ObjectType::Mask mask)
    {
        assert(!mask || (mask & ObjectType::kBaseMask)); (void)mask;
        invalidateParent(Node::kHasPendedActivation);
    }

    template<typename Derived, typename ObjectType>
    void SceneImpl::Leaf<Derived, ObjectType>::invalidateParent(ObjectType::Mask mask)
    {
        if (auto parent = _parent.lock(); parent)
        {
            parent->invalidate(mask);
        }
    }

    // SceneImpl::*Leaf //

    void SceneImpl::DirectionalLightLeaf::revalidate(Mask mask)
    {
        if (invalidatedAny(mask & kColor))
        {
            // a value has changed (probably a new epoch started)
            Lerpable::update(_scene._epochNumber, model());
        }

        Object::revalidate(mask);
    }

    void SceneImpl::PointLightLeaf::revalidate(Mask mask)
    {
        if (invalidatedAny(mask & (kColor | kAttenuation)))
        {
            // a value has changed (probably a new epoch started)
            Lerpable::update(_scene._epochNumber, model());
        }

        Object::revalidate(mask);
    }

    void SceneImpl::PerspectiveCameraLeaf::revalidate(Mask mask)
    {
        if (invalidatedAny(mask & (kYFov /*| kZNear | kZFar | kAspectRatio*/)))
        {
            // a value has changed (probably a new epoch started)
            Lerpable::update(_scene._epochNumber, model());
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
        }

        Object::revalidate(mask);
    }

    void SceneImpl::OrthographicCameraLeaf::activate()
    {
        _scene.setActiveCamera(*this);
    }

    // SceneImpl::Node //

    scene::Node::Sptr SceneImpl::Node::make(std::string const & name, models::Node model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        Node::Sptr node = std::make_shared<Node>(name, std::move(model), weak_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, node);
        MINIRE_INVARIANT(inserted, "failed to insert \"{}\" into \"{}\"", name, this->name());
        node->invalidate(kGlobalTransformDirty);
        return node;
    }

    scene::Mesh::Sptr SceneImpl::Node::make(std::string const & name, models::Mesh model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto mesh = _scene._rasterizer.meshes().getMesh(model._source,
                                                        model._defaultMaterial);
        assert(mesh);

        auto meshLeaf = std::make_shared<MeshLeaf>(name, model, weak_from_this(), mesh, _scene);
        auto [_, inserted] = _children.emplace(name, meshLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert \"{}\" into \"{}\"", name, this->name());
        _scene._meshLeaves.push_back(meshLeaf);

        if (model._skin)
        {
            models::Mesh::Skin::Bones const & bones = model._skin->_bones;
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

            meshLeaf->_skinOrigin = model._skin->_origin ? nodeFromPointer(*model._skin->_origin)
                                                         : Node::Wptr();
        }
        return meshLeaf;
    }

    scene::DirectionalLight::Sptr SceneImpl::Node::make(std::string const & name,
                                                        models::DirectionalLight model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto directionalLightLeaf = std::make_shared<DirectionalLightLeaf>(name, model, weak_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, directionalLightLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());
        _scene._directionalLightLeaves.push_back(directionalLightLeaf);
        return directionalLightLeaf;
    }

    scene::PointLight::Sptr SceneImpl::Node::make(std::string const & name,
                                                  models::PointLight model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto pointLightLeaf = std::make_shared<PointLightLeaf>(name, model, weak_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, pointLightLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());
        _scene._pointLightLeaves.push_back(pointLightLeaf);
        return pointLightLeaf;
    }

    scene::PerspectiveCamera::Sptr SceneImpl::Node::make(std::string const & name,
                                                         models::PerspectiveCamera model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto perspectiveCameraLeaf = std::make_shared<PerspectiveCameraLeaf>(name, model, weak_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, perspectiveCameraLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());
        return perspectiveCameraLeaf;
    }

    scene::OrthographicCamera::Sptr SceneImpl::Node::make(std::string const & name,
                                                          models::OrthographicCamera model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto orthographicCameraLeaf = std::make_shared<OrthographicCameraLeaf>(name, model, weak_from_this(), _scene);
        auto [_, inserted] = _children.emplace(name, orthographicCameraLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());
        return orthographicCameraLeaf;
    }

    scene::Billboard::Sptr SceneImpl::Node::make(std::string const & name,
                                                 models::Billboard model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto billboard = _scene._rasterizer.billboards().create(model);
        assert(billboard);

        auto billboardLeaf = std::make_shared<BillboardLeaf>(name, model, weak_from_this(), billboard, _scene);
        auto [_, inserted] = _children.emplace(name, billboardLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", name, this->name());

        _scene._billboardsLeaves.emplace_back(model._zOrder, billboardLeaf);
        _scene._billboardsLeaves.sort([](auto const & a, auto const & b)
            {
                return a.first < b.first;
            });
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
        if (_activeAnimation)
        {
            _activeAnimation.reset();
        }

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

    void SceneImpl::Node::playAnimation(models::AnimationId const & animationId,
                                        size_t repeats, float speedScale)
    {
        auto it = _animationSet.find(animationId);
        MINIRE_INVARIANT(it != _animationSet.cend(),
                         "no such animation: {}", animationId);

        assert(it->second);
        _activeAnimation = std::make_unique<ActiveAnimation>(
            it->second, repeats, speedScale);
        invalidate(kAnimation);
    }

    void SceneImpl::Node::stopAnimation()
    {
        if (_activeAnimation)
        {
            _activeAnimation.reset();
        }
    }

    void SceneImpl::Node::inlineAnimation(models::AnimationTracks animationTracks,
                                          size_t repeats, float speedScale)
    {
        _activeAnimation = std::make_unique<ActiveAnimation>(
            instantiateTracks(animationTracks), repeats, speedScale);
        invalidate(kAnimation);
    }

    // TODO: when parent is changed, some animation may stil refer to moved nodes,
    //       it will work, but ill-logic.
    void SceneImpl::Node::setParent(scene::Node::Sptr const & newParentIface)
    {
        SceneImpl::setParent(*this, newParentIface);
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

    SceneImpl::Node::SceneItem
    SceneImpl::Node::find(models::ScenePath const & path) const
    {
        if (auto it = findIterator(path); !it.empty())
        {
            return std::visit(
                [](auto const & child) -> SceneItem { return child; },
                it.item());
        }
        return std::monostate();
    }

    SceneImpl::Node::Node(std::string name,
                          Object::ModelType && model,
                          Wptr parent,
                          SceneImpl & scene)
        : scene::Node(std::move(name), std::move(model))
        , _scene(scene)
        , _localTransform(origin())
        , _parent(parent)
    {
        // calling at the end, to avoid unwanted calls to virtual methods
        Object::propagate(); // must be called before "setAllowPropagation" !
        setAllowPropagation(true);
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
        if (!_activeAnimation)
            return false;

        ActiveAnimation & activeAnimation = *_activeAnimation;

        // advance all sequencers (that aren't done)
        for(auto sequencer : activeAnimation._uniqueSequencers)
        {
            assert(sequencer);
            if (!sequencer->isDone())
            {
                sequencer->advance(_scene._frameTime);
            }
        }

        // update transformation
        assert(activeAnimation._animationTracks);
        assert(activeAnimation._animationSequencers.size() == activeAnimation._animationTracks->size());
        for(size_t i = 0; i < activeAnimation._animationTracks->size(); ++i)
        {
            AnimationTrack & animationTrack = (*activeAnimation._animationTracks)[i];
            ActiveAnimation::SequencerSet const & sequencerSet = activeAnimation._animationSequencers[i];

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
                    targetNode->_localTransform.update(_scene._epochNumber, current);
                    targetNode->invalidate(kGlobalTransformDirty);
                }
            }
        }

        // test if all sequencers are done
        bool hasNonDoneSequencer = std::ranges::any_of(
            activeAnimation._uniqueSequencers,
            [](ActiveAnimation::Sequencer::Sptr const & sequencer)
            {
                assert(sequencer);
                return !sequencer->isDone();
            });

        if (!hasNonDoneSequencer)
        {
            _activeAnimation.reset();
        }

        return hasNonDoneSequencer;
    }

    void SceneImpl::Node::revalidate(Mask mask)
    {
        Mask newMask = 0;
        Mask dropMask = 0;

        auto parent = _parent.lock();

        // Animation

        if (mask & kAnimation) // is it an Animation pass?
        {
            if (advanceAnimation())
            {
                // animation can be advanced again
                newMask |= kAnimation;
            }
            else
            {
                // animation is terminated
                dropMask |= kAnimation;
            }
        }

        // Lerping

        if (invalidatedAny(mask & (kHasPendedActivation | kOrigin)))
        {
            if (invalidatedAny(mask & kOrigin))
            {
                _localTransform.update(_scene._epochNumber, origin());
                newMask |= kHasActivateChildren;

                dropMask |= kOrigin;
            }
            dropMask |= kHasPendedActivation;
        }

        if (invalidatedAny(mask & kHasActivateChildren))
        {
            if (lerp(_scene._lerpWeight, _scene._epochNumber))
            {
                newMask |= (kHasActivateChildren | kGlobalTransformDirty);
            }
            else
            {
                dropMask |= kHasActivateChildren;
            }
        }

        // Global transform

        if (invalidatedAny(mask & kGlobalTransformDirty))
        {
            static const glm::mat4 kIdentityMatrix(glm::identity<glm::mat4>());
            static const glm::vec4 kGlobalOrigin(0, 0, 0, 1);

            models::Transform const & localTransform = _localTransform.current();

            glm::mat4 localTransformMatrix = localTransform.matrix();
            assert(!parent || parent->hasGlobalTransform());
            glm::mat4 const & parentGlobalTransform = parent ? parent->_globalTransform
                                                             : kIdentityMatrix;
            _globalTransform = parentGlobalTransform * localTransformMatrix;
            _globalPosition = _globalTransform * kGlobalOrigin; // will drop "w"

            dropMask |= kGlobalTransformDirty;
            invalidateChildren(kGlobalTransformDirty);
        }

        if (invalidatedAny(mask & kGlobalTransformGray))
        {
            dropMask |= kGlobalTransformGray;
        }

        // Visibility

        if (invalidatedAny(mask & kChildVisibilityInvalidated))
        {
            // just drop the flag
            dropMask |= kChildVisibilityInvalidated;
        }

        if (invalidatedAny(mask & (kVisible | kParentVisibilityInvalidated)))
        {
            bool const oldEffectiveVisible = _effectiveVisible;
            _effectiveVisible = visible() && (parent ? parent->_effectiveVisible : true);
            if (oldEffectiveVisible != _effectiveVisible)
            {
                invalidateChildren(kParentVisibilityInvalidated);
            }
            dropMask |= (kParentVisibilityInvalidated | kVisible);
        }

        Object::revalidate(dropMask);
        invalidate(newMask);
    }

    void SceneImpl::Node::propagate(Mask mask)
    {
        assert(invalidatedAll(mask));

        // don't propagate model's flags to parents
        Mask newParentMask = mask & ~kBaseMask;

        if (mask & kOrigin)
        {
            newParentMask |= kHasPendedActivation;
        }

        if (mask & kVisible)
        {
            newParentMask |= kChildVisibilityInvalidated;
        }

        if (mask & kGlobalTransformDirty)
        {
            newParentMask |= kGlobalTransformGray;
            newParentMask &= ~kGlobalTransformDirty;
        }

        // propagate flags upwards (own flags and by-pass ones)
        invalidateParent(newParentMask);
    }

    // NOTE: won't propagate upwards, only updates flags of given children
    void SceneImpl::Node::invalidateChildren(Mask mask)
    {
        for(auto const & [_, child] : _children)
        {
            if (Node::Sptr const * node = std::get_if<Node::Sptr>(&child);
                node)
            {
                assert(*node);
                (*node)->invalidate(mask, false);
            }
        }
    }

    // TODO: make it non-recursive
    void SceneImpl::Node::invalidateParent(Mask mask)
    {
        if (mask)
        {
            if(auto parent = _parent.lock();
                parent && !parent->invalidatedAll(mask))
            {
                parent->invalidate(mask);
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

    // Scene //

    SceneImpl::SceneImpl(Rasterizer & rasterizer)
        : _rasterizer(rasterizer)
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
        _root = std::make_shared<Node>("(the root)",
            models::Node{models::Transform{}, true},
            Node::Wptr(), *this);
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

    void SceneImpl::revalidate(Node * root, Node::Mask mask)
    {
        std::vector<Node *> queue{root};
        queue.reserve(_nodesEstimate);
        while (!queue.empty())
        {
            // fetch a node
            Node * node = queue.back();
            assert(node);
            queue.pop_back();

            // revalidate the node itself
            // (it may set/drop children flags)
            if (node->invalidatedAny(mask))
            {
                node->revalidate(mask);
            }

            // schedule children for revalidation
            bool setHasActivateLeaf = false;
            for(auto & [_, child] : node->_children)
            {
                std::visit(utils::Overloaded
                {
                    [&queue, mask](Node::Sptr & childNode)
                    {
                        assert(childNode);
                        if (childNode->invalidatedAny(mask))
                        {
                            queue.emplace_back(childNode.get());
                        }
                    },
                    [this, mask, &setHasActivateLeaf](auto & leaf)
                    {
                        assert(leaf);
                        if ((Node::kHasPendedActivation & mask) &&
                            leaf->invalidated())
                        {
                            leaf->revalidate(mask);
                        }

                        if (Node::kHasActivateChildren & mask)
                        {
                            if (leaf->lerp(_lerpWeight, _epochNumber))
                            {
                                setHasActivateLeaf |= true;
                            }
                        }
                    },
                }, child);
            }

            if (setHasActivateLeaf)
            {
                node->invalidate(Node::kHasActivateChildren);
            }

            _nodesEstimate = std::max(_nodesEstimate, queue.size());
        }
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

        // 1. advance animable objects

        _frameTime = frameTime;
        revalidate(_root.get(), Node::kAnimation);

        // 2. advance directly set values for a new Epoch or
        //    perform lerping for continuing Epoch

        if (epochStarted)
        {
            // transfer accumulated models state into scene instances
            revalidate(_root.get(), Node::kHasPendedActivation | Node::kOrigin);
        }

        _lerpWeight = epochDuration != 0 ? epochTime / epochDuration : 1.0;
        assert(_lerpWeight >= 0);
        revalidate(_root.get(), Node::kHasActivateChildren);

        // 3. revalidate effective values
        //    (transforms, viewport, visibility, etc)

        revalidate(_root.get(), Node::kVisible |
                                Node::kParentVisibilityInvalidated |
                                Node::kChildVisibilityInvalidated);

        revalidate(_root.get(), Node::kGlobalTransformDirty |
                                Node::kGlobalTransformGray);

        actualizeViewpoint();

#       ifndef NDEBUG
        // for debug-only: ensure that no nodes has been left invalidated
        std::vector<Node const *> queue;
        queue.reserve(_nodesEstimate);
        while(!queue.empty())
        {
            Node const * node = queue.back();
            queue.pop_back();

            // NOTE: Node::kAnimation may be stored between iterations
            assert(node);
            assert(node->invalidatedAny(Node::kAnimation) ||
                   !node->invalidated());

            for(auto & [_, child] : node->_children)
            {
                std::visit(utils::Overloaded
                {
                    [&queue](Node::Sptr & childNode)
                    {
                        assert(childNode);
                        queue.emplace_back(childNode.get());
                    },
                    [](auto & leaf) { assert(leaf && !leaf->invalidated()); },
                }, child);
            }
        }
#       endif
    }

    // TODO: should be a static method?
    material::SkinningVector
    SceneImpl::makeSkinningVector(MeshLeaf const & mesh) const
    {
        static const glm::mat4 kIdentityMatrix(glm::identity<glm::mat4>());

        material::SkinningVector result;
        result.reserve(mesh._skinBones.size());

        for(MeshLeaf::SkinBone const & skinBone : mesh._skinBones)
        {
            if (auto const & node = skinBone._node.lock();
                node && node->hasGlobalTransform())
            {
                result.emplace_back(node->_globalTransform * skinBone._inverseBindMatrix);
            }
            else
            {
                // TODO: add more debug info
                result.emplace_back(kIdentityMatrix);
                MINIRE_WARNING("skinBone refers to a non-existing Node or "
                               "its's global transform isn't clear");
            }
        }

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
        if (newParent)
        {
            auto [_, inserted] = newParent->_children.emplace(oldIterator->first,
                                                              oldIterator->second);
            MINIRE_INVARIANT(inserted, "failed to insert \"{}\" into a new parent node (\"{}\")",
                             item.name(), newParent->name());
        }

        // reset _parent for the element
        item._parent = newParent;

        // erase the element from an old parent
        oldParent->_children.erase(oldIterator);

        if (newParent)
        {
            item.propagate();
        }

    }
}
