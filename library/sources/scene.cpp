#include <scene.hpp>

#include <rasterizer.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/geometry.hpp>
#include <utils/overloaded.hpp>

#include <glm/gtx/transform.hpp>
#include <fmt/ranges.h>

#include <algorithm>
#include <cassert>
#include <vector>

namespace minire
{
    // Node //

    Scene::Node::Node(Scene & scene, models::Transform transform,
                      Wptr parent, bool visible)
        : _scene(scene)
        , _localTransform(transform)
        , _parent(parent)
        , _visible(visible)
    {
        ++_scene._nodesEstimate;
    }

    Scene::Node::~Node()
    {
        assert(_scene._nodesEstimate != 0);
        if (_scene._nodesEstimate != 0)
        {
            --_scene._nodesEstimate;
        }
    }

    bool Scene::Node::lerp(float weight, size_t epochNumber)
    {
        return _localTransform.lerp(weight, epochNumber);
    }

    // ActiveAnimation //

    Scene::ActiveAnimation::ActiveAnimation(AnimationTracksSptr const & animationTracks,
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

    Scene::Scene(Rasterizer & rasterizer)
        : _rasterizer(rasterizer)
    {
        handle(events::controller::SceneReset{});
    }

    void Scene::handle(events::controller::SceneReset const &)
    {
        _root = std::make_shared<Node>(*this, models::Transform{},
                                       Node::Wptr(), true);
    }

    void Scene::handle(events::controller::SceneDispose const & e)
    {
        if (e._item.empty())
        {
            handle(events::controller::SceneReset{});
        }

        auto [parent, iterator] = find<ChildIterator>(e._item);
        assert(parent && iterator != parent->_children.cend());
        parent->_children.erase(iterator);
    }

    void Scene::handle(events::controller::SceneActivateCamera const & e)
    {
        if (e._item.empty())
        {
            _activeCamera = std::monostate();
            _viewpoint.unsetCamera();
        }
        else
        {
            Node::Child camera = find<Node::Child>(e._item);
            _activeCamera = std::visit(utils::Overloaded
            {
                [this](PerspectiveCameraLeaf::Sptr & camera) -> ActiveCamera
                {
                    assert(camera);
                    _viewpoint.setCamera(camera->current());
                    return PerspectiveCameraLeaf::Wptr(camera);
                },

                [this](OrthographicCameraLeaf::Sptr & camera) -> ActiveCamera
                {
                    assert(camera);
                    _viewpoint.setCamera(camera->current());
                    return OrthographicCameraLeaf::Wptr(camera);
                },

                [&e](auto && item) -> ActiveCamera
                {
                    using T = std::decay_t<decltype(item)>;
                    MINIRE_THROW("got {} instead of a camera: {}",
                                 utils::demangle<T>(), e._item);
                }
            }, camera);
        }
    }

    void Scene::handle(events::controller::SceneNewNode const & e)
    {
        Node::Sptr parent = find<Node::Sptr>(e._parent);
        assert(parent);
        auto [_, inserted] = parent->_children.emplace(
            e._id, std::make_shared<Node>(*this, std::move(e._origin), parent, e._visible));
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", e._id, e._parent);
    }
    
    void Scene::handle(events::controller::SceneNewMesh const & e)
    {
        auto meshToken = _rasterizer.meshes().getMesh(e._data._source,
                                                      e._data._defaultMaterial);
        assert(meshToken);

        Node::Sptr parent = find<Node::Sptr>(e._parent);
        assert(parent);
        auto meshLeaf = std::make_shared<MeshLeaf>(std::move(meshToken), parent, e._visible);
        auto [_, inserted] = parent->_children.emplace(e._id, meshLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", e._id, e._parent);
        _meshLeaves.push_back(meshLeaf);
    }

    void Scene::handle(events::controller::SceneNewPointLight const & e)
    {
        Node::Sptr parent = find<Node::Sptr>(e._parent);
        assert(parent);
        auto pointLightLeaf = std::make_shared<PointLightLeaf>(e._data, parent, e._visible);
        auto [_, inserted] = parent->_children.emplace(e._id, pointLightLeaf);
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", e._id, e._parent);
        _pointLightLeaves.push_back(pointLightLeaf);
    }

    void Scene::handle(events::controller::SceneNewPerspectiveCamera const & e)
    {
        Node::Sptr parent = find<Node::Sptr>(e._parent);
        assert(parent);
        auto [_, inserted] = parent->_children.emplace(
            e._id, std::make_shared<PerspectiveCameraLeaf>(e._data, parent, e._visible));
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", e._id, e._parent);
    }

    void Scene::handle(events::controller::SceneNewOrthographicCamera const & e)
    {
        Node::Sptr parent = find<Node::Sptr>(e._parent);
        assert(parent);
        auto [_, inserted] = parent->_children.emplace(
            e._id, std::make_shared<OrthographicCameraLeaf>(e._data, parent, e._visible));
        MINIRE_INVARIANT(inserted, "failed to insert {} into {}", e._id, e._parent);
    }
    
    void Scene::handle(events::controller::SceneSetParent const & e)
    {
        // Find an item's iterator and its parent
        // (it must exists unless event._item points to the _root)
        auto [oldParent, oldIterator] = find<ChildIterator>(e._item);
        assert(oldParent && oldIterator != oldParent->_children.cend());

        // Find a new parent and insert element to a new parent's children
        Node::Sptr newParent = find<Node::Sptr>(e._attribute);
        assert(newParent);
        auto [newIt, inserted] = newParent->_children.emplace(oldIterator->first,
                                                              oldIterator->second);
        MINIRE_INVARIANT(inserted, "failed to insert {} into new parent: {}",
                         e._item, e._attribute);

        // Reset _parent for the element
        std::visit(utils::Overloaded
        {
            [&oldParent, &newParent](auto & child)
            {
                assert(child);
                assert(child->_parent.lock() == oldParent);
                child->_parent = newParent;
            }
        }, newIt->second);

        // Erase the element from an old parent
        oldParent->_children.erase(oldIterator);
    }
    
    void Scene::handle(events::controller::SceneSetVisibility const & e)
    {
        Node::Child item = find<Node::Child>(e._item);
        std::visit(
            [v = e._attribute](auto & item) { assert(item); item->_visible = v; },
            item);

        // TODO: drop lerp target when switching false -> true
    }

    void Scene::handle(events::controller::SceneSetTransform const & e,
                       size_t epochNumber)
    {
        auto node = find<Node::Sptr>(e._item);
        assert(node);
        node->_localTransform.update(epochNumber, e._attribute);
        activate(*node);
    }
    
    void Scene::handle(events::controller::SceneSetPointLight const & e,
                       size_t epochNumber)
    {
        auto pointLight = find<PointLightLeaf::Sptr>(e._item);
        assert(pointLight);
        pointLight->update(epochNumber, e._attribute);
        activate(*pointLight);
    }

    void Scene::handle(events::controller::SceneSetPerspectiveCamera const & e,
                       size_t epochNumber)
    {
        auto perspectiveCamera = find<PerspectiveCameraLeaf::Sptr>(e._item);
        assert(perspectiveCamera);
        perspectiveCamera->update(epochNumber, e._attribute);
        activate(*perspectiveCamera);
    }

    void Scene::handle(events::controller::SceneSetOrthographicCamera const & e,
                       size_t epochNumber)
    {
        auto orthographicCamera = find<OrthographicCameraLeaf::Sptr>(e._item);
        assert(orthographicCamera);
        orthographicCamera->update(epochNumber, e._attribute);
        activate(*orthographicCamera);
    }

    void Scene::handle(events::controller::SceneNewAnimationSet const & e)
    {
        Node::Sptr containerNode = find<Node::Sptr>(e._containerNode);
        assert(containerNode);

        // drop any current active animation
        if (containerNode->_activeAnimation)
        {
            containerNode->_activeAnimation.reset();
            deactiveChildrenAnimation(containerNode->_parent.lock());
        }

        // transform animation set from abstract (model) into a concrete one
        AnimationSet newAnimationSet;
        newAnimationSet.reserve(e._animationSet.size());
        for(auto const & [animationId, animationTracks] : e._animationSet)
        {
            AnimationTracksSptr animationTracksSptr = std::make_shared<AnimationTracks>();
            animationTracksSptr->reserve(animationTracks.size());
            for(auto const & [targetScenePath, keyframeAnimation] : animationTracks)
            {
                Node::Sptr targetNode = find<Node::Sptr>(models::concat(e._containerNode, targetScenePath));
                assert(targetNode);
                animationTracksSptr->emplace_back(AnimationTrack
                {
                    ._target = targetNode,
                    ._animation = scene::makeKeyframeAnimation(keyframeAnimation),
                });
            }
            newAnimationSet.emplace(animationId, animationTracksSptr);
        }

        containerNode->_animationSet = std::move(newAnimationSet);

        /*
            PLAN 2:
                - add an AnimationSet (Animations tokes, Channels mapping, Sequencer) into a Node
                    - Anims will be targeted only to the underlying Nodes (children)
                - preload node's animations (AnimationSet) at glTF instantiator (w/ optional flag)
                - SceneAnimationPlay will be targeter by a node w/ AnimationSet
                    - exluclusive for now, but in future may be added
                      several Sequencers w/ Mixers and Transitions
                - Maybe add ShortCut (size_t)
                - Only Duplication instancing mode
                - Spare buffers?
        */
    }

    void Scene::handle(events::controller::ScenePlayAnimation const & e)
    {
        Node::Sptr node = find<Node::Sptr>(e._containerNode);
        assert(node);
        auto it = node->_animationSet.find(e._animationId);
        MINIRE_INVARIANT(it != node->_animationSet.cend(),
                         "no such animation: {}", e._animationId);

        assert(it->second);
        node->_activeAnimation = std::make_unique<ActiveAnimation>(
            it->second, e._repeats, e._speedScale);
        activeChildrenAnimation(node->_parent.lock());
    }

    void Scene::handle(events::controller::SceneStopAnimation const & e)
    {
        Node::Sptr node = find<Node::Sptr>(e._containerNode);
        assert(node);
        if (node->_activeAnimation)
        {
            node->_activeAnimation.reset();
            deactiveChildrenAnimation(node->_parent.lock());
        }
    }

    // TODO: cover with tests
    // - empty
    // - "node"
    // - "leaf"
    // - "node"/"node"
    // - "node"/"leaf"
    // - "not-exist"
    // - "node"/"not-exist"
    // - "leaf"/"anything"
    template<typename T>
    T Scene::find(models::ScenePath const & path)
    {
        static_assert(std::is_same_v<T, Node::Child> ||
                      std::is_same_v<T, Node::Sptr> ||
                      std::is_same_v<T, PointLightLeaf::Sptr> ||
                      std::is_same_v<T, PerspectiveCameraLeaf::Sptr> ||
                      std::is_same_v<T, OrthographicCameraLeaf::Sptr> ||
                      std::is_same_v<T, ChildIterator>,
                      "unexpected result type");
        T result;
        Node::Child current = _root;
        for(std::string const & component : path)
        {
            if (Node::Sptr * asNode = std::get_if<Node::Sptr>(&current);
                asNode != nullptr)
            {
                assert(*asNode);
                if (auto it = (*asNode)->_children.find(component);
                    it != (*asNode)->_children.cend())
                {
                    if constexpr(std::is_same_v<T, ChildIterator>)
                    {
                        assert(std::holds_alternative<Node::Sptr>(current));
                        result.first = std::get<Node::Sptr>(current);
                        result.second = it;
                    }
                    current = it->second;
                }
                else
                {
                    // no such path element found
                    MINIRE_THROW("no such scene element: {}", path);
                }
            }
            else
            {
                // some leaf is alredy met, the path continues
                MINIRE_THROW("no such scene element: {}", path);
            }
        }

        if constexpr(std::is_same_v<T, Node::Sptr> ||
                     std::is_same_v<T, PointLightLeaf::Sptr> ||
                     std::is_same_v<T, PerspectiveCameraLeaf::Sptr> ||
                     std::is_same_v<T, OrthographicCameraLeaf::Sptr>)
        {
            T * fetched = std::get_if<T>(&current);
            MINIRE_INVARIANT(fetched, "unexpected element at path \"{}\": {}",
                             path, utils::demangle<T>());
            result = *fetched;
            assert(result);
        }
        else if constexpr(!std::is_same_v<T, ChildIterator>)
        {
            result = current;
        }

        if constexpr (std::is_same_v<T, Node::Child>)
        {
            assert(std::visit([](auto const & i){ return i.operator bool(); }, result));
        }

        return result;
    }

    void Scene::setViewport(size_t weight, size_t height)
    {
        _viewpoint.setViewport(weight, height);
    }

    template<typename T>
    void Scene::activate(T & item)
    {
        item._activated = true;
        activateParents(item._parent.lock());
    }

    void Scene::activateParents(Node::Sptr parent)
    {
        while(parent && !parent->_childActivated)
        {
            parent->_childActivated = true;
            parent = parent->_parent.lock();
        }
    }

    void Scene::activeChildrenAnimation(Node::Sptr parent)
    {
        while(parent && !parent->_hasActiveChildrenAnimation)
        {
            parent->_hasActiveChildrenAnimation = true;
            parent = parent->_parent.lock();
        }
    }

    void Scene::deactiveChildrenAnimation(Node::Sptr parent)
    {
        while(parent && parent->_hasActiveChildrenAnimation)
        {
            parent->_hasActiveChildrenAnimation = std::any_of(
                parent->_children.cbegin(), parent->_children.cend(),
                [](auto const & pair)
                {
                    return std::visit(utils::Overloaded
                    {
                        [](Node::Sptr const & i) { return i->_activeAnimation.operator bool(); },
                        [](auto const &) { return false; },
                    }, pair.second);
                });
            parent = parent->_parent.lock();
        }
    }

    // TODO: don't animate invisible nodes
    // TODO: don't animate culled-out nodes
    bool Scene::advanceAnimations(float delta /* seconds */, size_t epochNumber)
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

                    Node::Sptr targetNode = animationTrack._target.lock();
                    if (!targetNode) continue;

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
                        targetNode->_localTransform.update(epochNumber, current);
                        activate(*targetNode);
                        updated |= true;
                    }
                }

                // maybe deactive (if all sequencers are done)
                bool hasNonDoneSequencer = std::any_of(
                    activeAnimation._uniqueSequencers.cbegin(),
                    activeAnimation._uniqueSequencers.cend(),
                    [](ActiveAnimation::Sequencer::Sptr const & sequencer)
                    {
                        assert(sequencer);
                        return !sequencer->isDone();
                    });
                if (hasNonDoneSequencer)
                {
                    activeChildrenAnimation(node->_parent.lock());
                }
                else
                {
                    node->_activeAnimation.reset();
                    deactiveChildrenAnimation(node->_parent.lock());
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

    // TODO: don't lerp invisible nodes
    // TODO: don't lerp culled-out nodes
    void Scene::lerp(float weight, size_t epochNumber)
    {
        assert(_root);
        std::vector<Node::Sptr> queue{_root};
        queue.reserve(_nodesEstimate);
        while(!queue.empty())
        {
            Node::Sptr node = queue.back();
            queue.pop_back();

            assert(node);

            if (node->_activated)
            {
                node->_activated = node->lerp(weight, epochNumber);
                node->_hasGlobalTransform = false;
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
                        [weight, epochNumber](auto & child) -> bool
                        {
                            assert(child);
                            return child->lerp(weight, epochNumber);
                        }
                    }, child);
                }
            }

            if (node->_activated || node->_childActivated)
            {
                activateParents(node->_parent.lock());
            }
        }

        updateGlobalTransforms();
        actualizeViewpoint();
    }

    // TODO: don't lerp invisible nodes
    // TODO: don't lerp culled-out nodes
    void Scene::updateGlobalTransforms()
    {
        static const glm::mat4 kIdentityMatrix(glm::identity<glm::mat4>());
        static const glm::vec4 kOrigin(0, 0, 0, 1);

        assert(_root);
        std::vector<Node::Sptr> queue{_root};
        queue.reserve(_nodesEstimate);
        while (!queue.empty())
        {
            Node::Sptr node = queue.back();
            queue.pop_back();

            assert(node);

            if (!node->_hasGlobalTransform)
            {
                models::Transform const & localTransform = node->_localTransform.current();
                glm::mat4 localTransformMatrix = glm::translate(localTransform._translation) *
                                                 glm::toMat4(localTransform._rotation) *
                                                 glm::scale(localTransform._scale);
                Node::Sptr parent = node->_parent.lock();
                assert(!parent || parent->_hasGlobalTransform);
                glm::mat4 const & parentGlobalTransform = parent ? parent->_globalTransform
                                                                 : kIdentityMatrix;
                node->_globalTransform = parentGlobalTransform * localTransformMatrix;
                node->_globalPosition = node->_globalTransform * kOrigin; // will drop "w"
                node->_hasGlobalTransform = true;
            }

            for(auto & [_, child] : node->_children)
            {
                std::visit(utils::Overloaded
                {
                    [&queue](Node::Sptr & node)
                    {
                        assert(node);
                        queue.push_back(node);
                    },
                    [](auto &) {},
                }, child);
            }
        }
    }

    void Scene::actualizeViewpoint()
    {
        std::visit(utils::Overloaded
        {
            [](std::monostate) {},
            [this](auto const & wcamera)
            {
                if (auto camera = wcamera.lock(); camera)
                {
                    Node::Sptr parent = camera->_parent.lock();
                    MINIRE_INVARIANT(parent, "an active camera has no parent");
                    MINIRE_INVARIANT(parent->_hasGlobalTransform,
                                     "an active camera's node has no global transform");
                    // TODO: don't update if transform didn't change
                    _viewpoint.setTransform(parent->_globalTransform,
                                            parent->_globalPosition);

                    // TODO: don't update unless camera or its parameters changed
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
}
