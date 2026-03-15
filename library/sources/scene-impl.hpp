#pragma once

#include <minire/content/id.hpp>
#include <minire/content/path.hpp>
#include <minire/errors.hpp>
#include <minire/material.hpp>
#include <minire/models/scene-path.hpp>
#include <minire/models/transform.hpp>
#include <minire/scene.hpp>

#include <scene-impl/animations.hpp>
#include <scene-impl/viewpoint.hpp>
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
     * TODO: why are they split? Maybe make a single public call
     * Main loop's expected call sequence:
     *
     *  revalidateModels()  // mandatory
     *  advanceAnimations() // optional
     *  lerp()              // optional (mandatory if animations advanced)
     *  revalidateNode()    // mandatory
     *  cullModels(...)
     *
     * */
    class SceneImpl
        : public Scene
    {
    public:
        explicit SceneImpl(Rasterizer &);

    public:
        scene::Node & root() const override;

        void setActiveCamera(models::ScenePath const &) override;

        void reset() override;

    public:
        void setViewport(size_t width, size_t height);

        scene::Viewpoint const & viewpoint() const { return _viewpoint; }

        void setEpochNumber(size_t epochNumber) { _epochNumber = epochNumber; }

        void lerp(float weight);

        void revalidateModels();

        void revalidateNodes();

        bool advanceAnimations(float delta /* seconds */);

    public:
        // TODO: implement detached() mechanism

        template<typename Callable>
        void cullModels(Callable callable) const
        {
            auto it = _meshLeaves.begin();
            while(it != _meshLeaves.end())
            {
                if (auto const & mesh = *it;
                    mesh && !mesh->detached())
                {
                    if (mesh->visible())
                    {
                        // estimate a pivot (origin) point of a model
                        auto parent = mesh->_skinOrigin;
                        if (!parent || parent->detached()) parent = mesh->_parent.lock();
                        MINIRE_INVARIANT(parent && !parent->detached(),
                                         "a mesh doesn't have a parent");
                        assert(parent->hasGlobalTransform());

                        // perform rendering
                        assert(mesh->_mesh);
                        callable(*mesh->_mesh, mesh->emissiveFactor(),
                                 parent->_globalTransform,
                                 makeSkinningVector(*mesh));
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
                if (auto const & billboard = it->second;
                    billboard && !billboard->detached())
                {
                    if (billboard->visible())
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
                if (auto const & directionalLight = *it;
                    directionalLight && !directionalLight->detached())
                {
                    if (directionalLight->visible())
                    {
                        auto parent = directionalLight->_parent.lock();
                        MINIRE_INVARIANT(parent, "a point light doesn't have a parent");
                        assert(parent->hasGlobalTransform());
                        // NOTE: 3-rd column of transform matrix is a z-axis direction
                        glm::vec3 const direction = glm::vec3(parent->_globalTransform[2]);
                        auto const & current = directionalLight->current();
                        callable(index,
                                 parent->_globalPosition, // TODO: it could be taken from _globalTransform
                                 glm::normalize(direction),
                                 current._color,
                                 current._shadowParams);
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
                if (auto const & pointLight = *it;
                    pointLight && !pointLight->detached())
                {
                    if (pointLight->visible())
                    {
                        auto parent = pointLight->_parent.lock();
                        MINIRE_INVARIANT(parent, "a point light doesn't have a parent");
                        assert(parent->hasGlobalTransform());
                        auto const & current = pointLight->current();
                        callable(index,
                                 parent->_globalPosition, // TODO: it could be taken from _globalTransform
                                 current._color,
                                 current._attenuation,
                                 current._shadowParams);
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

    private:
        void updateGlobalTransforms();
        void actualizeViewpoint();

    private:
        class Node;

        template<typename Derived,
                 typename ObjectType>
        class Leaf
            : public ObjectType
            , public std::enable_shared_from_this<Derived>
        {
        public:
            using Sptr = std::shared_ptr<Derived>;

            using ObjectType::detach;
            using ObjectType::name;

            explicit Leaf(std::string name,
                          typename ObjectType::ModelType const & model,
                          std::weak_ptr<Node> parent)
                : ObjectType(std::move(name), model)
                , _parent(parent)
            {}

            scene::Node::Wptr parent() const override { return _parent; }
            void setParent(scene::Node::Sptr const & newParent) override;

        private:
            void propagate() override { if (auto p = _parent.lock(); p) p->propagate(); }

        private:
            std::weak_ptr<Node> _parent;
            bool                _activated = false;

            friend class SceneImpl;
        };

        // TODO: maybe model() shouldn't be stored at all (if it is only used at ctor)?
        class MeshLeaf final
            : public Leaf<MeshLeaf, scene::Mesh>
        {
        public:
            explicit MeshLeaf(std::string name,
                              models::Mesh const & model,
                              std::weak_ptr<Node> parent,
                              std::shared_ptr<rasterizer::Mesh> const & mesh)
                : Leaf(std::move(name), model, parent)
                , _mesh(mesh)
            {}

            bool lerp(float, size_t) { return false; } // just for compatibility

            // TODO: lerpable _emissiveFactor

        private:
            struct SkinBone
            {
                glm::mat4 const       _inverseBindMatrix;
                std::shared_ptr<Node> _node; // TODO: too many detached() checks
            };
            using SkinBones = std::vector<SkinBone>;

            std::shared_ptr<rasterizer::Mesh> _mesh;
            SkinBones                         _skinBones;
            std::shared_ptr<Node>             _skinOrigin; // TODO: too many detached() checks

            friend class SceneImpl;
        };

        template<typename Derived, typename SceneType, typename ModelType>
        class LerpableLeaf
            : public Leaf<Derived, SceneType>
            , public utils::Lerpable<ModelType>
        {
        public:
            explicit LerpableLeaf(std::string name,
                                 ModelType const & model,
                                 std::weak_ptr<Node> parent,
                                 SceneImpl & scene)
                : Leaf<Derived, SceneType>(std::move(name), model, parent)
                , utils::Lerpable<ModelType>(model)
                , _scene(scene)
            {}

        protected:
            SceneImpl & _scene;
        };

        class DirectionalLightLeaf final
            : public LerpableLeaf<DirectionalLightLeaf,
                                  scene::DirectionalLight,
                                  models::DirectionalLight>
        {
        public:
            using LerpableLeaf::LerpableLeaf;

            void revalidate() override;
        };

        class PointLightLeaf final
            : public LerpableLeaf<PointLightLeaf,
                                  scene::PointLight,
                                  models::PointLight>
        {
        public:
            using LerpableLeaf::LerpableLeaf;

            void revalidate() override;
        };

        class PerspectiveCameraLeaf final
            : public LerpableLeaf<PerspectiveCameraLeaf,
                                  scene::PerspectiveCamera,
                                  models::PerspectiveCamera>
        {
        public:
            using LerpableLeaf::LerpableLeaf;

            void activate() override;
            void revalidate() override;
        };

        class OrthographicCameraLeaf final
            : public LerpableLeaf<OrthographicCameraLeaf,
                                  scene::OrthographicCamera,
                                  models::OrthographicCamera>
        {
        public:
            using LerpableLeaf::LerpableLeaf;

            void activate() override;
            void revalidate() override;
        };

        class BillboardLeaf final
            : public Leaf<BillboardLeaf, scene::Billboard>
        {
        public:
            explicit BillboardLeaf(std::string name,
                                   models::Billboard model,
                                   std::weak_ptr<Node> parent,
                                   std::shared_ptr<rasterizer::Billboard> const & billboard)
                : Leaf(std::move(name), std::move(model), parent)
                , _billboard(billboard)
            {}

            bool lerp(float, size_t) { return false; } // just for compatibility

            auto const & billboard() const { return _billboard; }

        private:
            std::shared_ptr<rasterizer::Billboard> _billboard;

            friend class SceneImpl;
        };

        void setActiveCamera(PerspectiveCameraLeaf & camera);
        void setActiveCamera(OrthographicCameraLeaf & camera);

    private:
        struct AnimationTrack
        {
            std::shared_ptr<Node>          _target; // TODO: too many detached() checks
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

        class Node final
            : public scene::Node
            , public std::enable_shared_from_this<Node>
        {
        public:
            using Sptr = std::shared_ptr<Node>;
            using Wptr = std::weak_ptr<Node>;

            Node(std::string name,
                 Object::ModelType && model,
                 Wptr parent,
                 SceneImpl & scene);

        public:
            scene::Node::Sptr make(std::string const & name, models::Node) override;
            scene::Mesh::Sptr make(std::string const & name, models::Mesh) override;
            scene::DirectionalLight::Sptr make(std::string const &, models::DirectionalLight) override;
            scene::PointLight::Sptr make(std::string const &, models::PointLight) override;
            scene::PerspectiveCamera::Sptr make(std::string const &, models::PerspectiveCamera) override;
            scene::OrthographicCamera::Sptr make(std::string const &, models::OrthographicCamera) override;
            scene::Billboard::Sptr make(std::string const &, models::Billboard) override;

            void makeFromSource(content::Path const &, content::Manager &, bool visible) override;

        public:
            void makeAnimationSet(models::AnimationSet animationSet) override;
            void playAnimation(models::AnimationId const &, size_t repeats, float speedScale) override;
            void stopAnimation() override;
            void inlineAnimation(models::AnimationTracks animationTracks,
                                 size_t repeats, float speedScale) override;

        public:
            size_t size() const override { return _children.size(); }
            bool empty() const override { return _children.empty(); }

            scene::Node::Wptr parent() const override { return _parent; }
            void setParent(scene::Node::Sptr const & newParent) override;

            void dispose(models::ScenePath const &) override;
            void disposeAll() override;

            void detach() override;

        private:
            SceneItem find(models::ScenePath const &) const override;

        private:
            bool lerp(float weight, size_t epochNumber);

            bool hasGlobalTransform() const
            {
                return GlobalTransformState::kClean == _globalTransformState;
            }

            void invalidateGlobalTransform();
            void deactiveChildrenAnimation();
            void activeChildrenAnimation();

            void revalidate() override;
            void propagate() override;

        private:
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

            SceneImpl           & _scene;
            LerpableTransform     _localTransform;
            glm::vec3             _globalPosition;
            glm::mat4             _globalTransform;
            Wptr                  _parent;
            ChildrenMap           _children;
            AnimationSet          _animationSet;
            ActiveAnimation::Uptr _activeAnimation;
            GlobalTransformState  _globalTransformState = GlobalTransformState::kDirty;
            bool                  _visible = true;
            bool                  _activated = false; // for _localTransform
            bool                  _childActivated = false;
            bool                  _hasActiveChildrenAnimation = false;
            bool                  _modelInvalidated = false; // Object::invalidated
            // TODO: too many flags, consider migration to std::bitset

        private:
            struct ItemIterator
            {
                Node const *                _parent;
                ChildrenMap::const_iterator _iterator;

                bool empty() const { return !_parent || _iterator == _parent->_children.cend(); }
                auto const & item() const { assert(!empty()); return _iterator->second; }
                void erase()
                {
                    assert(!empty());
                    const_cast<Node *>(_parent)->_children.erase(_iterator);
                }
            };

            ItemIterator findIterator(models::ScenePath const &) const;

            // NOTE: empty path points to the _root,
            // Will throw if path incorrect, otherwise result guranteed to correct.
            template<typename T>
            typename T::Sptr const & findInternal(models::ScenePath const &) const;

            Node::Sptr nodeFromPointer(models::NodePointer const &) const;

            friend class SceneImpl;
        };

        using ActiveCamera = std::variant<std::monostate,
                                          PerspectiveCameraLeaf::Sptr,
                                          OrthographicCameraLeaf::Sptr>;

    private:
        using BillboardsLeafRecord = std::pair<size_t, BillboardLeaf::Sptr>;

        using MeshLeaves = std::list<MeshLeaf::Sptr>;
        using BillboardsLeaves = std::list<BillboardsLeafRecord>;
        using DirectionalLightLeaves = std::list<DirectionalLightLeaf::Sptr>;
        using PointLightLeaves = std::list<PointLightLeaf::Sptr>;

        template<typename T>
        void activate(T &);

        void activateParents(Node::Sptr);

        material::SkinningVector makeSkinningVector(MeshLeaf const &) const;

        // TODO: lerpable _ambientLight

    private:
        Rasterizer                   & _rasterizer;
        Node::Sptr                     _root;
        ActiveCamera                   _activeCamera;
        scene::Viewpoint               _viewpoint;
        size_t                         _epochNumber = 0;
        size_t                         _nodesEstimate = 1;

        mutable MeshLeaves             _meshLeaves;
        mutable BillboardsLeaves       _billboardsLeaves;
        mutable DirectionalLightLeaves _directionalLightLeaves;
        mutable PointLightLeaves       _pointLightLeaves;

        friend class Node;
    };
}
