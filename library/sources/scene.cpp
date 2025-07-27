#include <scene.hpp>

#include <rasterizer.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/geometry.hpp>
#include <utils/overloaded.hpp>

#include <glm/gtx/transform.hpp>
#include <fmt/ranges.h>

#include <cassert>
#include <vector>

namespace minire
{
    // Node //

    Scene::Node::Node(models::Transformation transformation,
                 Wptr parent, bool visible)
        : _localTransformation(transformation)
        , _parent(parent)
        , _visible(visible)
    {}

    bool Scene::Node::lerp(float weight, size_t epochNumber)
    {
        return _localTransformation.lerp(weight, epochNumber);
    }

    // Scene //

    Scene::Scene(Rasterizer & rasterizer)
        : _rasterizer(rasterizer)
    {
        handle(events::controller::SceneReset{});
    }

    void Scene::handle(events::controller::SceneReset const &)
    {
        _root = std::make_shared<Node>(models::Transformation{},
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
            e._id, std::make_shared<Node>(std::move(e._origin), parent, e._visible));
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
        // TODO: mark as active this and their parent
    }

    void Scene::handle(events::controller::SceneSetTransformation const & e,
                       size_t epochNumber)
    {
        auto node = find<Node::Sptr>(e._item);
        assert(node);
        node->_localTransformation.update(epochNumber, e._attribute);
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
    T Scene::find(events::controller::ScenePath const & path)
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

    // TODO: don't lerp invisible nodes
    // TODO: don't lerp culled-out nodes
    void Scene::lerp(float weight, size_t epochNumber)
    {
        assert(_root);
        std::vector<Node::Sptr> queue{_root};
        // TODO: reserve estimate for Nodes count
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
    // TODO: maybe do a lazy re-calc global transforms?
    void Scene::updateGlobalTransforms()
    {
        static const glm::mat4 kIdentityMatrix(glm::identity<glm::mat4>());
        static const glm::vec4 kOrigin(0, 0, 0, 1);

        assert(_root);
        std::vector<Node::Sptr> queue{_root};
        // TODO: reserve estimate for Nodes count
        while (!queue.empty())
        {
            Node::Sptr node = queue.back();
            queue.pop_back();

            assert(node);

            if (!node->_hasGlobalTransform)
            {
                models::Transformation const & localTransform = node->_localTransformation.current();
                glm::mat4 localTransformMatrix = glm::translate(localTransform._translation) *
                                                 glm::toMat4(localTransform._rotation) *
                                                 glm::scale(localTransform._scale);
                Node::Sptr parent = node->_parent.lock();
                assert(!parent || parent->_hasGlobalTransform);
                glm::mat4 const & parentGlobalTransform = parent ? parent->_globalTransformation
                                                                 : kIdentityMatrix;
                node->_globalTransformation = parentGlobalTransform * localTransformMatrix;
                node->_globalPosition = node->_globalTransformation * kOrigin; // will drop "w"
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
                    _viewpoint.setTransform(parent->_globalTransformation,
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
