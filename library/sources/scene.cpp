// TODO: this class is a mess, should refactor it

#include <scene.hpp>

#include <rasterizer.hpp>
#include <rasterizer/constants.hpp>
#include <scene/gltf-instantiator.hpp>
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
    // TODO: store/calculate absolute path for nodes/leaves (and use them for logging)

    // SceneImpl::Leaf //

    template<typename Derived, typename ObjectType>
    void SceneImpl::Leaf<Derived, ObjectType>::setParent(scene::Node::Sptr const & newParentIface)
    {
        // find a leaf's iterator
        auto oldParent = _parent.lock();
        MINIRE_INVARIANT(oldParent, "an un-parented scene leaf cannot be re-parented: \"{}\"", name());
        auto oldIterator = oldParent->_children.find(name());

        // find a new parent and insert element to a new parent's children
        SceneImpl::Node::Sptr newParent = std::static_pointer_cast<SceneImpl::Node>(newParentIface);
        if (newParent)
        {
            auto [_, inserted] = newParent->_children.emplace(oldIterator->first,
                                                              oldIterator->second);
            MINIRE_INVARIANT(inserted, "failed to insert \"{}\" into a new parent node (\"{}\")",
                             name(), newParent->name());
        }
        else
        {
            // if a new parent isn't specified, a leaf should be detached from a scene
            detach();
        }

        // reset _parent for the element
        _parent = newParent;

        // erase the element from an old parent
        oldParent->_children.erase(oldIterator);

        if (newParent)
        {
            if (ObjectType::invalidated()) propagate();
            // TODO: also should re-activate other stuff of a parent 
        }
    }

    // SceneImpl::*Leaf //

    void SceneImpl::DirectionalLightLeaf::revalidate()
    {
        if (invalidated(kColor))
        {
            Lerpable::update(_scene._epochNumber, model());
            _scene.activate(*this); // TODO: why?
        }
        Object::revalidate();
    }

    void SceneImpl::PointLightLeaf::revalidate()
    {
        if (invalidated(kColor | kAttenuation))
        {
            Lerpable::update(_scene._epochNumber, model());
            _scene.activate(*this); // TODO: why?
        }
        Object::revalidate();
    }

    void SceneImpl::PerspectiveCameraLeaf::revalidate()
    {
        if (invalidated(kYFov /*| kZNear | kZFar | kAspectRatio*/))
        {
            Lerpable::update(_scene._epochNumber, model());
            _scene.activate(*this);
        }
        Object::revalidate();
    }

    void SceneImpl::PerspectiveCameraLeaf::activate()
    {
        _scene.setActiveCamera(*this);
    }

    void SceneImpl::OrthographicCameraLeaf::revalidate()
    {
        if (invalidated(kXMag | kYMag /* | kZNear | kZFar*/))
        {
            Lerpable::update(_scene._epochNumber, model());
            _scene.activate(*this);
        }
        Object::revalidate();
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
        invalidateGlobalTransform();
        return node;
    }
    
    scene::Mesh::Sptr SceneImpl::Node::make(std::string const & name, models::Mesh model)
    {
        MINIRE_INVARIANT(!name.empty(), "a name is empty");
        auto mesh = _scene._rasterizer.meshes().getMesh(model._source,
                                                        model._defaultMaterial);
        assert(mesh);

        auto meshLeaf = std::make_shared<MeshLeaf>(name, model, weak_from_this(), mesh);
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
                meshLeaf->_skinBones.emplace_back(MeshLeaf::SkinBone
                {
                    ._inverseBindMatrix = boneModel._inverseBindMatrix,
                    ._node = nodeFromPointer(boneModel._jointNode),
                });
                MINIRE_INVARIANT(meshLeaf->_skinBones.back()._node,
                                 "skin bone is a null pointer: {}", boneModel._jointNode);
            }

            meshLeaf->_skinOrigin = model._skin->_origin ? nodeFromPointer(*model._skin->_origin)
                                                         : Node::Sptr();
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

        auto billboardLeaf = std::make_shared<BillboardLeaf>(name, model, weak_from_this(), billboard);
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

    void SceneImpl::Node::makeAnimationSet(models::AnimationSet animationSet) // TODO: const ref?
    {
        // drop any current active animation
        if (_activeAnimation)
        {
            _activeAnimation.reset();
            deactiveChildrenAnimation();
        }

        // transform animation set from abstract (model) into a concrete one
        AnimationSet newAnimationSet;
        newAnimationSet.reserve(animationSet.size());
        for(auto const & [animationId, animationTracks] : animationSet)
        {
            // TODO: code dup
            AnimationTracksSptr animationTracksSptr = std::make_shared<AnimationTracks>();
            animationTracksSptr->reserve(animationTracks.size());
            for(auto const & [target, keyframeAnimation] : animationTracks)
            {
                animationTracksSptr->emplace_back(AnimationTrack
                {
                    ._target = nodeFromPointer(target),
                    ._animation = scene::makeKeyframeAnimation(keyframeAnimation),
                });
                MINIRE_INVARIANT(animationTracksSptr->back()._target &&
                                 !animationTracksSptr->back()._target->detached(),
                                 "animation target is a null pointer: {}", target);
            }
            newAnimationSet.emplace(animationId, animationTracksSptr);
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
        activeChildrenAnimation();
    }
    
    void SceneImpl::Node::stopAnimation()
    {
        if (_activeAnimation)
        {
            _activeAnimation.reset();
            deactiveChildrenAnimation();
        }
    }
    
    void SceneImpl::Node::inlineAnimation(models::AnimationTracks animationTracks,
                                          size_t repeats, float speedScale)
    {
        // transform animation set from abstract (model) into a concrete one
        // TODO: code dup
        AnimationTracksSptr animationTracksSptr = std::make_shared<AnimationTracks>();
        animationTracksSptr->reserve(animationTracks.size());
        for(auto const & [target, keyframeAnimation] : animationTracks)
        {
            animationTracksSptr->emplace_back(AnimationTrack
            {
                ._target = nodeFromPointer(target),
                ._animation = scene::makeKeyframeAnimation(keyframeAnimation),
            });
            MINIRE_INVARIANT(animationTracksSptr->back()._target &&
                             !animationTracksSptr->back()._target->detached(),
                             "animation target is a null pointer: {}", target);
        }

        // activate this animation
        _activeAnimation = std::make_unique<ActiveAnimation>(
            animationTracksSptr, repeats, speedScale);
        activeChildrenAnimation();
    }

    // TODO: code duplicated w/ Left::setParent
    void SceneImpl::Node::setParent(scene::Node::Sptr const & newParentIface)
    {
        // Find a leaf's iterator
        auto oldParent = _parent.lock();
        MINIRE_INVARIANT(oldParent, "an un-parented scene leaf cannot be re-parented: \"{}\"", name());
        auto oldIterator = oldParent->_children.find(name());

        // Find a new parent and insert element to a new parent's children
        SceneImpl::Node::Sptr newParent = std::static_pointer_cast<SceneImpl::Node>(newParentIface);
        if (newParent)
        {
            auto [_, inserted] = newParent->_children.emplace(oldIterator->first,
                                                              oldIterator->second);
            MINIRE_INVARIANT(inserted, "failed to insert \"{}\" into a new parent node (\"{}\")",
                             name(), newParent->name());
        }
        else
        {
            // if a new parent isn't specified, a leaf should be detached from a scene
            detach();
        }

        // Reset _parent for the element
        _parent = newParent;

        // Erase the element from an old parent
        oldParent->_children.erase(oldIterator);

        if (newParent)
        {
            if (invalidated()) propagate();
            // TODO: also should re-activate other stuff of a parent
        }
    }

    void SceneImpl::Node::dispose(models::ScenePath const & path)
    {
        if (auto it = findIterator(path); !it.empty())
        {
            // TODO: Node::detach must detach all children recursively
            std::visit([](auto & item) { assert(item); item->detach(); },
                       it.item());            
            it.erase();

            // TODO: erase animations and other assiciated objects
        }
    }

    void SceneImpl::Node::disposeAll()
    {
        _children.clear();
        // TODO: erase animations and other assiciated objects
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
        , _localTransform(origin()) // TODO: duplicate w/ model()?
        , _parent(parent)
        , _visible(visible())       // TODO: duplicate w/ model()?
    {
        ++_scene._nodesEstimate;

        Node::propagate();

        // calling at the end, to avoid unwanted calls to virtual methods
        setAllowPropagation(true);
    }

    SceneImpl::Node::~Node()
    {
        assert(_scene._nodesEstimate != 0);
        if (_scene._nodesEstimate != 0)
        {
            --_scene._nodesEstimate;
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

    void SceneImpl::Node::invalidateGlobalTransform()
    {
        _globalTransformState = GlobalTransformState::kDirty;
        for(auto node = _parent.lock();
            node && GlobalTransformState::kClean == node->_globalTransformState;
            node = node->_parent.lock())
        {
            node->_globalTransformState = GlobalTransformState::kGrey;
        }
    }

    void SceneImpl::Node::deactiveChildrenAnimation()
    {
        for(auto parent = _parent.lock();
            parent && parent->_hasActiveChildrenAnimation;
            parent = parent->_parent.lock())
        {
            parent->_hasActiveChildrenAnimation = std::any_of(
                parent->_children.cbegin(), parent->_children.cend(),
                [](auto const & pair)
                {
                    return std::visit(utils::Overloaded
                    {
                        [](Node::Sptr const & i)
                        {
                            assert(i);
                            return i->_hasActiveChildrenAnimation ||
                                   i->_activeAnimation.operator bool();
                        },
                        [](auto const &) { return false; },
                    }, pair.second);
                });
        }
    }

    void SceneImpl::Node::activeChildrenAnimation()
    {
        for(auto parent = _parent.lock();
            parent && !parent->_hasActiveChildrenAnimation;
            parent = parent->_parent.lock())
        {
            parent->_hasActiveChildrenAnimation = true;
        }
    }

    void SceneImpl::Node::revalidate()
    {
        if (invalidated(kOrigin))
        {
            _localTransform.update(_scene._epochNumber, origin());
            _scene.activate(*this); // NOTE: will also invalidate global transform
        }
        Object::revalidate();
    }

    void SceneImpl::Node::propagate()
    {
        _modelInvalidated = true;
        for(auto parent = _parent.lock();
            parent && !parent->_modelInvalidated;
            parent = parent->_parent.lock())
        {
            parent->_modelInvalidated = true;
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
        MINIRE_INVARIANT(!camera.detached(), "the camera is detached: {}", camera.name());
        _activeCamera = ActiveCamera(camera.shared_from_this());
        _viewpoint.setCamera(camera.current());
    }

    // TODO: code duplication
    void SceneImpl::setActiveCamera(OrthographicCameraLeaf & camera)
    {
        MINIRE_INVARIANT(!camera.detached(), "the camera is detached: {}", camera.name());
        _activeCamera = camera.shared_from_this();
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

    // TODO: don't revalidate invisible nodes
    // TODO: don't revalidate culled-out nodes
    void SceneImpl::revalidateModels()
    {
        std::vector<Node::Sptr> queue;
        queue.reserve(_nodesEstimate);  // TODO: it should be "max-depth-estimate" rather than nodes count

        assert(_root);
        if (_root->_modelInvalidated)
            queue.emplace_back(_root);

        while(!queue.empty())
        {
            Node::Sptr node = queue.back();
            queue.pop_back();

            assert(node);
            node->revalidate();
            node->_modelInvalidated = false;

            for(auto & [_, child] : node->_children)
            {
                std::visit(utils::Overloaded
                {
                    [&queue](Node::Sptr const & childNode)
                    {
                        assert(childNode);
                        if (childNode->_modelInvalidated)
                        {
                            queue.emplace_back(childNode);
                        }
                    },
                    [this](auto const & leafNode)
                    {
                        assert(leafNode);
                        leafNode->revalidate();
                    },
                }, child);
            }
        }
    }

    // TODO: don't animate invisible nodes
    // TODO: don't animate culled-out nodes
    bool SceneImpl::advanceAnimations(float delta /* seconds */)
    {
        assert(_root);
        std::vector<Node::Sptr> queue{_root};
        queue.reserve(_nodesEstimate);

        bool updated = false;
        while(!queue.empty())
        {
            Node::Sptr node = queue.back();
            queue.pop_back();

            assert(node);

            if (node->_activeAnimation)
            {
                ActiveAnimation & activeAnimation = *node->_activeAnimation;

                // advance all sequencers (that aren't done)
                for(auto sequencer : activeAnimation._uniqueSequencers)
                {
                    assert(sequencer);
                    if (!sequencer->isDone())
                    {
                        sequencer->advance(delta);
                    }
                }

                // update transformation
                assert(activeAnimation._animationTracks);
                assert(activeAnimation._animationSequencers.size() == activeAnimation._animationTracks->size());
                for(size_t i = 0; i < activeAnimation._animationTracks->size(); ++i)
                {
                    AnimationTrack const & animationTrack = (*activeAnimation._animationTracks)[i];
                    ActiveAnimation::SequencerSet const & sequencerSet = activeAnimation._animationSequencers[i];

                    Node::Sptr const & targetNode = animationTrack._target;
                    if (!targetNode || targetNode->detached()) continue;

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
                        targetNode->_localTransform.update(_epochNumber, current);
                        activate(*targetNode);
                        updated |= true;
                    }
                }

                // maybe deactive (if all sequencers are done)
                bool hasNonDoneSequencer = std::any_of( // TODO: ranges
                    activeAnimation._uniqueSequencers.cbegin(),
                    activeAnimation._uniqueSequencers.cend(),
                    [](ActiveAnimation::Sequencer::Sptr const & sequencer)
                    {
                        assert(sequencer);
                        return !sequencer->isDone();
                    });
                if (hasNonDoneSequencer)
                {
                    node->activeChildrenAnimation();
                }
                else
                {
                    node->_activeAnimation.reset();
                    node->deactiveChildrenAnimation();
                }
            }

            if (node->_hasActiveChildrenAnimation)
            {
                for(auto & [_, child] : node->_children)
                {
                    if (auto * nodePtr = std::get_if<Node::Sptr>(&child);
                        nodePtr)
                    {
                        Node::Sptr node = *nodePtr;
                        assert(node);
                        if (node->_hasActiveChildrenAnimation ||
                            node->_activeAnimation.operator bool())
                        {
                            queue.emplace_back(node);
                        }
                    }
                }
            }
        }

        return updated;
    }

    template<typename T>
    void SceneImpl::activate(T & item)
    {
        item._activated = true;
        activateParents(item._parent.lock());
    }

    void SceneImpl::activateParents(Node::Sptr parent)
    {
        while(parent && !parent->_childActivated)
        {
            parent->_childActivated = true;
            parent = parent->_parent.lock();
        }
    }

    // TODO: don't lerp invisible nodes
    // TODO: don't lerp culled-out nodes
    void SceneImpl::lerp(float weight)
    {
        assert(_root);

        if (!_root->_activated && !_root->_childActivated)
            return;

        std::vector<Node::Sptr> queue{_root};
        queue.reserve(_nodesEstimate);
        while(!queue.empty())
        {
            Node::Sptr node = queue.back();
            queue.pop_back();

            assert(node);

            if (node->_activated)
            {
                node->_activated = node->lerp(weight, _epochNumber);
                node->invalidateGlobalTransform();
            }

            if (node->_childActivated)
            {
                node->_childActivated = false;
                for(auto & [_, child] : node->_children)
                {
                    node->_childActivated |= std::visit(utils::Overloaded
                    {
                        [&queue](Node::Sptr & child) -> bool
                        {
                            assert(child);
                            queue.emplace_back(child);
                            return false; // lerping status is not known yet
                        },
                        [weight, this](auto & child) -> bool
                        {
                            assert(child);
                            return child->lerp(weight, _epochNumber);
                        }
                    }, child);
                }
            }

            if (node->_activated || node->_childActivated)
            {
                activateParents(node->_parent.lock());
            }
        }
    }

    // TODO: don't lerp invisible nodes
    // TODO: don't lerp culled-out nodes
    void SceneImpl::updateGlobalTransforms()
    {
        static const glm::mat4 kIdentityMatrix(glm::identity<glm::mat4>());
        static const glm::vec4 kOrigin(0, 0, 0, 1);

        assert(_root);

        if (Node::GlobalTransformState::kClean == _root->_globalTransformState)
            return;

        std::vector<Node::Sptr> queue{_root};
        queue.reserve(_nodesEstimate);
        while (!queue.empty())
        {
            Node::Sptr node = queue.back();
            queue.pop_back();

            assert(node);

            // actualize own global transform (TODO: move into Node::)
            if (Node::GlobalTransformState::kDirty == node->_globalTransformState)
            {
                models::Transform const & localTransform = node->_localTransform.current();
                glm::mat4 localTransformMatrix = localTransform.matrix();
                Node::Sptr parent = node->_parent.lock();
                assert(!parent || parent->_globalTransformState == Node::GlobalTransformState::kClean);
                glm::mat4 const & parentGlobalTransform = parent ? parent->_globalTransform
                                                                 : kIdentityMatrix;
                node->_globalTransform = parentGlobalTransform * localTransformMatrix;
                node->_globalPosition = node->_globalTransform * kOrigin; // will drop "w"
            }

            // maybe schedule children actualization (TODO: move into Node::)
            if (Node::GlobalTransformState::kClean != node->_globalTransformState)
            {
                bool const forceDirty = Node::GlobalTransformState::kDirty == node->_globalTransformState;
                for(auto & [_, child] : node->_children)
                {
                    if (Node::Sptr * pnode = std::get_if<Node::Sptr>(&child); pnode)
                    {
                        Node::Sptr node = *pnode;
                        assert(node);
                        if (forceDirty)
                        {
                            node->_globalTransformState = Node::GlobalTransformState::kDirty;
                        }
                        if (Node::GlobalTransformState::kClean != node->_globalTransformState)
                        {
                            queue.push_back(node);
                        }
                    }
                }
            }

            // mark node as clean (TODO: move into Node::)
            node->_globalTransformState = Node::GlobalTransformState::kClean;
        }
    }

    void SceneImpl::revalidateNodes()
    {
        updateGlobalTransforms();
        actualizeViewpoint();
    }

    void SceneImpl::actualizeViewpoint()
    {
        std::visit(utils::Overloaded
        {
            [](std::monostate) {},
            [this](auto const & camera)
            {
                if (camera && !camera->detached())
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

    // TODO: should be a static method?
    material::SkinningVector
    SceneImpl::makeSkinningVector(MeshLeaf const & mesh) const
    {
        static const glm::mat4 kIdentityMatrix(glm::identity<glm::mat4>());

        material::SkinningVector result;
        result.reserve(mesh._skinBones.size());

        for(MeshLeaf::SkinBone const & skinBone : mesh._skinBones)
        {
            if (skinBone._node && !skinBone._node->detached() &&
                skinBone._node->hasGlobalTransform())
            {
                result.emplace_back(skinBone._node->_globalTransform * skinBone._inverseBindMatrix);
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
}
