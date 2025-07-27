#pragma once

#include <minire/content/id.hpp>
#include <minire/content/path.hpp>
#include <minire/errors.hpp>
#include <minire/events/controller/scene.hpp>
#include <minire/models/transformation.hpp>

#include <scene/viewpoint.hpp>
#include <utils/lerpable.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace minire::content { class Manager; }
namespace minire::rasterizer { class MeshToken; }

namespace minire
{
    class Rasterizer;

    class Scene
    {
    public:
        explicit Scene(Rasterizer &);

    public:
        void handle(events::controller::SceneReset const &);
        void handle(events::controller::SceneDispose const &);
        void handle(events::controller::SceneActivateCamera const &);
        void handle(events::controller::SceneNewNode const &);
        void handle(events::controller::SceneNewMesh const &);
        void handle(events::controller::SceneNewPointLight const &);
        void handle(events::controller::SceneNewPerspectiveCamera const &);
        void handle(events::controller::SceneNewOrthographicCamera const &);
        void handle(events::controller::SceneSetParent const &);
        void handle(events::controller::SceneSetVisibility const &);
        void handle(events::controller::SceneSetTransformation const &, size_t epochNumber);
        void handle(events::controller::SceneSetPointLight const &, size_t epochNumber);
        void handle(events::controller::SceneSetPerspectiveCamera const &, size_t epochNumber);
        void handle(events::controller::SceneSetOrthographicCamera const &, size_t epochNumber);

    public:
        void setViewport(size_t width, size_t height);

        scene::Viewpoint const & viewpoint() const { return _viewpoint; }

        void lerp(float weight, size_t epochNumber);

    public:
        template<typename Callable>
        void cullModels(Callable callable) const
        {
            auto it = _meshLeaves.begin();
            auto end = _meshLeaves.end();
            while(it != end)
            {
                if (auto mesh = it->lock();
                    mesh)
                {
                    if (mesh->_visible)
                    {
                        auto parent = mesh->_parent.lock();
                        MINIRE_INVARIANT(parent, "a point light doesn't have a parent");
                        assert(parent->_hasGlobalTransform);
                        assert(mesh->_meshToken);
                        callable(*mesh->_meshToken, parent->_globalTransformation);
                    }

                    ++it;
                }
                else
                {
                    it = _meshLeaves.erase(it);
                    end = _meshLeaves.end();
                }
            }
        }

        template<typename Callable>
        size_t cullPointLights(size_t limit, Callable callable) const
        {
            // TODO: sort by "front-to-back"
            // TODO: sort by distance and cull the farest

            size_t index = 0;
            auto it = _pointLightLeaves.begin();
            auto end = _pointLightLeaves.end();
            while(it != end && index < limit)
            {
                if (auto pointLight = it->lock();
                    pointLight)
                {
                    if (pointLight->_visible)
                    {
                        auto parent = pointLight->_parent.lock();
                        MINIRE_INVARIANT(parent, "a point light doesn't have a parent");
                        assert(parent->_hasGlobalTransform);
                        callable(index,
                                 parent->_globalPosition,
                                 pointLight->current()._color,
                                 pointLight->current()._attenuation);
                        ++index;
                    }
                    ++it;
                }
                else
                {
                    it = _pointLightLeaves.erase(it);
                    end =  _pointLightLeaves.end();
                }
            }
            return index;
        }

    private:
        void updateGlobalTransforms();
        void actualizeViewpoint();

    private:
        struct Node;

        template<typename T>
        struct Leaf : public T
        {
            std::weak_ptr<Node> _parent;
            bool                _visible = true;
            bool                _activated = false;

            using Sptr = std::shared_ptr<Leaf>;
            using Wptr = std::weak_ptr<Leaf>;

            Leaf(typename T::ElementType const & init,
                 std::weak_ptr<Node> parent,
                 bool visible)
                : T(init)
                , _parent(parent)
                , _visible(visible)
            {}
        };

        struct Mesh
        {
            using ElementType = std::shared_ptr<rasterizer::MeshToken>;

            ElementType _meshToken;

            explicit Mesh(ElementType meshToken)
                : _meshToken(std::move(meshToken))
            {}

            bool lerp(float, size_t) { return false; } // just for compatibility
        };

        using MeshLeaf = Leaf<Mesh>;
        using PointLightLeaf = Leaf<utils::Lerpable<models::PointLight>>;
        using PerspectiveCameraLeaf = Leaf<utils::Lerpable<models::PerspectiveCamera>>;
        using OrthographicCameraLeaf = Leaf<utils::Lerpable<models::OrthographicCamera>>;

        struct Node
        {
            using Sptr = std::shared_ptr<Node>;
            using Wptr = std::weak_ptr<Node>;

            using Child = std::variant<Node::Sptr,
                                       MeshLeaf::Sptr,
                                       PointLightLeaf::Sptr,
                                       PerspectiveCameraLeaf::Sptr,
                                       OrthographicCameraLeaf::Sptr>;

            using ChildrenMap = std::unordered_map<std::string, Child>;
            using LerpableTransformation = utils::Lerpable<models::Transformation>;

            // TODO: rename Transformation -> Transform

            LerpableTransformation _localTransformation;
            glm::vec3              _globalPosition;
            glm::mat4              _globalTransformation;
            Wptr                   _parent;
            ChildrenMap            _children;
            bool                   _visible = true;
            bool                   _activated = false; // for _localTransformation
            bool                   _childActivated = false;
            bool                   _hasGlobalTransform = false;

            Node(models::Transformation, Wptr parent, bool visible);

            bool lerp(float weight, size_t epochNumber);
        };

        using ActiveCamera = std::variant<std::monostate,
                                          PerspectiveCameraLeaf::Wptr,
                                          OrthographicCameraLeaf::Wptr>;

    private:
        using PointLightLeaves = std::vector<PointLightLeaf::Wptr>;
        using MeshLeaves = std::vector<MeshLeaf::Wptr>;
        using ChildIterator = std::pair<Node::Sptr, Node::ChildrenMap::iterator>;

        // NOTE: empty path points to the _root,
        // Will throw if path incorrect, otherwise result guranteed to correct.
        template<typename T>
        T find(events::controller::ScenePath const &);

        template<typename T>
        void activate(T &);

        void activateParents(Node::Sptr);

    private:
        Rasterizer     & _rasterizer;
        Node::Sptr       _root;
        ActiveCamera     _activeCamera;
        scene::Viewpoint _viewpoint;
mutable PointLightLeaves _pointLightLeaves;
mutable MeshLeaves       _meshLeaves;
    };
}
