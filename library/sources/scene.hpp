#pragma once

#include <minire/content/id.hpp>
#include <minire/content/path.hpp>
#include <minire/errors.hpp>
#include <minire/events/controller/scene.hpp>
#include <minire/models/scene-path.hpp>
#include <minire/models/transform.hpp>

#include <scene/animations.hpp>
#include <scene/viewpoint.hpp>
#include <utils/lerpable.hpp>

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace minire::content { class Manager; }
namespace minire::rasterizer { class Billboard; }
namespace minire::rasterizer { class Mesh; }

namespace minire
{
    class Rasterizer;

    /**
     * Main loop's expected call sequence:
     *
     *  advanceAnimations() // optional
     *  lerp()              // optional (mandatory if animations advanced)
     *  revalidateNode()    // mandatory
     *  cullModels(...)
     *
     * */
    class Scene
    {
    public:
        explicit Scene(Rasterizer &);

    public:
        void handle(events::controller::SceneReset const &);
        void handle(events::controller::SceneDispose const &);
        void handle(events::controller::SceneActivateCamera const &);
        void handle(events::controller::SceneSetAmbientLight const &);
        void handle(events::controller::SceneNewNode const &);
        void handle(events::controller::SceneNewMesh const &);
        void handle(events::controller::SceneNewDirectionalLight const &);
        void handle(events::controller::SceneNewPointLight const &);
        void handle(events::controller::SceneNewPerspectiveCamera const &);
        void handle(events::controller::SceneNewOrthographicCamera const &);
        void handle(events::controller::SceneNewBillboard const &);
        void handle(events::controller::SceneSetParent const &);
        void handle(events::controller::SceneSetVisibility const &);
        void handle(events::controller::SceneSetTransform const &, size_t epochNumber);
        void handle(events::controller::SceneSetDirectionalLight const &, size_t epochNumber);
        void handle(events::controller::SceneSetPointLight const &, size_t epochNumber);
        void handle(events::controller::SceneSetPerspectiveCamera const &, size_t epochNumber);
        void handle(events::controller::SceneSetOrthographicCamera const &, size_t epochNumber);
        void handle(events::controller::SceneSetMeshEmissiveFactor const &);
        void handle(events::controller::SceneNewAnimationSet const &);
        void handle(events::controller::ScenePlayAnimation const &);
        void handle(events::controller::SceneStopAnimation const &);

    public:
        void setViewport(size_t width, size_t height);

        scene::Viewpoint const & viewpoint() const { return _viewpoint; }

        void lerp(float weight, size_t epochNumber);

        void revalidateNodes();

        bool advanceAnimations(float delta /* seconds */, size_t epochNumber);

    public:
        template<typename Callable>
        void cullModels(Callable callable) const
        {
            auto it = _meshLeaves.begin();
            while(it != _meshLeaves.end())
            {
                if (auto mesh = it->lock();
                    mesh)
                {
                    if (mesh->_visible)
                    {
                        auto parent = mesh->_parent.lock();
                        MINIRE_INVARIANT(parent, "a point light doesn't have a parent");
                        assert(parent->hasGlobalTransform());
                        assert(mesh->_mesh);
                        callable(*mesh->_mesh, mesh->_emissiveFactor,
                                 parent->_globalTransform);
                    }

                    ++it;
                }
                else
                {
                    it = _meshLeaves.erase(it);
                }
            }
        }

        template<typename Callable>
        void cullBillboards(Callable callable) const
        {
            auto it = _billboardsLeaves.begin();
            while(it != _billboardsLeaves.end())
            {
                if (auto billboard = it->second.lock();
                    billboard)
                {
                    if (billboard->_visible)
                    {
                        auto parent = billboard->_parent.lock();
                        MINIRE_INVARIANT(parent, "a billboard doesn't have a parent");
                        assert(parent->hasGlobalTransform());
                        assert(billboard->_billboard);
                        callable(*billboard->_billboard, parent->_globalTransform);
                    }

                    ++it;
                }
                else
                {
                    it = _billboardsLeaves.erase(it);
                }
            }
        }

        template<typename Callable>
        size_t cullDirectionalLights(size_t limit, Callable callable) const
        {
            // TODO: sort by "front-to-back"
            // TODO: sort by distance and cull the farest

            size_t index = 0;
            auto it = _directionalLightLeaves.begin();
            while(it != _directionalLightLeaves.end() && index < limit)
            {
                if (auto directionalLight = it->lock();
                    directionalLight)
                {
                    if (directionalLight->_visible)
                    {
                        auto parent = directionalLight->_parent.lock();
                        MINIRE_INVARIANT(parent, "a point light doesn't have a parent");
                        assert(parent->hasGlobalTransform());
                        // NOTE: 3-rd column of transform matrix is a z-axis direction
                        glm::vec3 const direction = glm::vec3(parent->_globalTransform[2]);
                        callable(index,
                                 parent->_globalPosition, // TODO: it could be taken from _globalTransform
                                 glm::normalize(direction),
                                 directionalLight->current()._color,
                                 directionalLight->current()._enableShadows);
                        ++index;
                    }
                    ++it;
                }
                else
                {
                    it = _directionalLightLeaves.erase(it);
                }
            }
            return index;
        }

        template<typename Callable>
        size_t cullPointLights(size_t limit, Callable callable) const
        {
            // TODO: sort by "front-to-back"
            // TODO: sort by distance and cull the farest

            size_t index = 0;
            auto it = _pointLightLeaves.begin();
            while(it != _pointLightLeaves.end() && index < limit)
            {
                if (auto pointLight = it->lock();
                    pointLight)
                {
                    if (pointLight->_visible)
                    {
                        auto parent = pointLight->_parent.lock();
                        MINIRE_INVARIANT(parent, "a point light doesn't have a parent");
                        assert(parent->hasGlobalTransform());
                        callable(index,
                                 parent->_globalPosition, // TODO: it could be taken from _globalTransform
                                 pointLight->current()._color,
                                 pointLight->current()._attenuation);
                        ++index;
                    }
                    ++it;
                }
                else
                {
                    it = _pointLightLeaves.erase(it);
                }
            }
            return index;
        }

        glm::vec3 const & ambientLight() const { return _ambientLight; }

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

            explicit Leaf(typename T::ElementType const & init,
                          std::weak_ptr<Node> parent,
                          bool visible)
                : T(init)
                , _parent(parent)
                , _visible(visible)
            {}
        };

        struct Mesh
        {
            using ElementType = std::shared_ptr<rasterizer::Mesh>;

            ElementType _mesh;
            glm::vec3   _emissiveFactor = glm::vec3(0); // TODO: is not lerpable?

            explicit Mesh(ElementType const & mesh)
                : _mesh(mesh)
            {}

            bool lerp(float, size_t) { return false; } // just for compatibility
        };

        struct Billboard
        {
            using ElementType = std::shared_ptr<rasterizer::Billboard>;

            ElementType _billboard;

            explicit Billboard(ElementType const & billboard)
                : _billboard(billboard)
            {}

            bool lerp(float, size_t) { return false; } // just for compatibility
        };

        using MeshLeaf = Leaf<Mesh>;
        using DirectionalLightLeaf = Leaf<utils::Lerpable<models::DirectionalLight>>;
        using PointLightLeaf = Leaf<utils::Lerpable<models::PointLight>>;
        using PerspectiveCameraLeaf = Leaf<utils::Lerpable<models::PerspectiveCamera>>;
        using OrthographicCameraLeaf = Leaf<utils::Lerpable<models::OrthographicCamera>>;
        using BillboardLeaf = Leaf<Billboard>;

        struct AnimationTrack
        {
            std::weak_ptr<Node>            _target;
            scene::KeyframeAnimation::Sptr _animation;
        };

        // NOTE: will guarantee that each _target appears only once
        using AnimationTracks = std::vector<AnimationTrack>;
        using AnimationTracksSptr = std::shared_ptr<AnimationTracks>;

        using AnimationSet = std::unordered_map<models::AnimationId,
                                                AnimationTracksSptr>;
        struct ActiveAnimation
        {
            using Uptr = std::unique_ptr<ActiveAnimation>;
            using Sequencer = utils::Sequencer<float>;

            struct SequencerSet
            {
                Sequencer::CSptr _translation;
                Sequencer::CSptr _rotation;
                Sequencer::CSptr _scale;
            };

            AnimationTracksSptr          _animationTracks;
            std::vector<SequencerSet>    _animationSequencers;
            std::vector<Sequencer::Sptr> _uniqueSequencers;

            ActiveAnimation(AnimationTracksSptr const &,
                            size_t const repeats,
                            float const speedScale);
        };

        struct Node
        {
            using Sptr = std::shared_ptr<Node>;
            using Wptr = std::weak_ptr<Node>;

            using Child = std::variant<Node::Sptr,
                                       MeshLeaf::Sptr,
                                       DirectionalLightLeaf::Sptr,
                                       PointLightLeaf::Sptr,
                                       PerspectiveCameraLeaf::Sptr,
                                       OrthographicCameraLeaf::Sptr,
                                       BillboardLeaf::Sptr>;

            using ChildrenMap = std::unordered_map<std::string, Child>;
            using LerpableTransform = utils::Lerpable<models::Transform>;

            enum class GlobalTransformState
            {
                kClean, // both own and every children's are up to date
                kDirty, // own transform is outdated
                kGrey,  // own is clean, but some children are outdated
            };

            Scene                 & _scene;
            LerpableTransform       _localTransform;
            glm::vec3               _globalPosition;
            glm::mat4               _globalTransform;
            Wptr                    _parent;
            ChildrenMap             _children;
            AnimationSet            _animationSet;
            ActiveAnimation::Uptr   _activeAnimation;
            GlobalTransformState    _globalTransformState = GlobalTransformState::kDirty;
            bool                    _visible = true;
            bool                    _activated = false; // for _localTransform
            bool                    _childActivated = false;
            bool                    _hasActiveChildrenAnimation = false;

            Node(Scene & scene, models::Transform,
                 Wptr parent, bool visible);

            ~Node();

            bool lerp(float weight, size_t epochNumber);

            bool hasGlobalTransform() const
            {
                return GlobalTransformState::kClean == _globalTransformState;
            }
        };

        using ActiveCamera = std::variant<std::monostate,
                                          PerspectiveCameraLeaf::Wptr,
                                          OrthographicCameraLeaf::Wptr>;

    private:
        using BillboardsLeafRecord = std::pair<size_t, BillboardLeaf::Wptr>;

        using MeshLeaves = std::list<MeshLeaf::Wptr>;
        using BillboardsLeaves = std::list<BillboardsLeafRecord>;
        using DirectionalLightLeaves = std::list<DirectionalLightLeaf::Wptr>;
        using PointLightLeaves = std::list<PointLightLeaf::Wptr>;

        using ChildIterator = std::pair<Node::Sptr, Node::ChildrenMap::iterator>;

        // NOTE: empty path points to the _root,
        // Will throw if path incorrect, otherwise result guranteed to correct.
        template<typename T>
        T find(models::ScenePath const &);

        template<typename T>
        void activate(T &);

        void activateParents(Node::Sptr);

        void activeChildrenAnimation(Node::Sptr);
        void deactiveChildrenAnimation(Node::Sptr);

        void invalidateGlobalTransform(Node::Sptr);

    private:
        Rasterizer                   & _rasterizer;
        Node::Sptr                     _root;
        ActiveCamera                   _activeCamera;
        scene::Viewpoint               _viewpoint;
        size_t                         _nodesEstimate = 1;
        glm::vec3                      _ambientLight = glm::vec3(0.03f);

        mutable MeshLeaves             _meshLeaves;
        mutable BillboardsLeaves       _billboardsLeaves;
        mutable DirectionalLightLeaves _directionalLightLeaves;
        mutable PointLightLeaves       _pointLightLeaves;

        friend class Node;
    };
}
