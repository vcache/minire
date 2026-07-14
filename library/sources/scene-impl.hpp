#pragma once

#include <minire/content/id.hpp>
#include <minire/content/path.hpp>
#include <minire/errors.hpp>
#include <minire/material.hpp>
#include <minire/models/scene-path.hpp>
#include <minire/models/transform.hpp>
#include <minire/scene.hpp>
#include <minire/scene/spatial-index.hpp>
#include <minire/utils/culling-test.hpp>

#include <material/types.hpp>
#include <rasterizer/mesh.hpp>
#include <scene-impl/animations.hpp>
#include <scene-impl/viewpoint.hpp>
#include <scene/spatial-handler.hpp>
#include <utils/lerpable.hpp>

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace minire::content { class Manager; }
namespace minire::rasterizer { class Billboard; }

namespace minire
{
    class Rasterizer;

    class SceneImpl
        : public Scene
    {
        class Node;
        class BillboardLeaf;

        static constexpr scene::IndexLayer kMeshLayer       = 0;
        static constexpr scene::IndexLayer kBillboardLayer  = 1;

    public:
        explicit SceneImpl(Rasterizer &);

    public:
        scene::Node & root() const override;

        void setActiveCamera(models::ScenePath const &) override;

        void reset() override;

        scene::SceneItem fetchSceneItem(size_t const x, size_t const y) const override;

        scene::SceneItem fetchHotSceneItem() const override;

        void setupSpatialIndex(scene::SpatialIndex::Uptr &&) override;

    public:
        void setViewport(size_t width, size_t height);

        scene::Viewpoint const & viewpoint() const { return _viewpoint; }

    public:
        void advance(size_t const epochNumber,
                     double const epochTime,         // seconds elapsed since an Epoch start
                     double const epochDuration,     // seconds duration of a (previous) Epoch
                     double const frameTime);        // seconds duration of a (previous) frame

    public:
        using OpbId = uint32_t; // NOTE: "0" is a special case (not an object or not used)

        auto const & pixelEdgeOutlines() const { return _pixelEdgeOutlines; }

        template<typename Callable>
        void cullModels(utils::FrustumPlanes const & frustumPlanes,
                        Callable callable) const
        {
            assert(_spatialIndex);
            _meshCullBuffer.clear();
            _spatialIndex->cull(frustumPlanes, kMeshLayer, _meshCullBuffer);
            for (void * opaque : _meshCullBuffer)
            {
                if (MeshLeaf * meshLeaf = static_cast<MeshLeaf *>(opaque);
                    meshLeaf && meshLeaf->visible())
                {
                    // estimate a pivot (origin) point of a model
                    auto parent = meshLeaf->_skinOrigin.lock();

                    // no valid skinOrigin, thus fallback to a parent
                    if (!parent) parent = meshLeaf->_parent.lock();

                    MINIRE_INVARIANT(parent, "a mesh doesn't have a parent");
                    assert(parent->hasGlobalTransform());
                    assert(meshLeaf->_mesh);

                    // TODO: is this cullingTest redundant and can be safely skipped?
                    if (parent->_effectiveVisible &&
                        utils::cullingTest(meshLeaf->worldAabb(), frustumPlanes))
                    {
                        // perform rendering
                        callable(*meshLeaf->_mesh, meshLeaf->emissiveFactor(),
                                 parent->_globalTransform, makeSkinningVector(*meshLeaf),
                                 meshLeaf->opbId());
                    }
                }
            }
        }

        template<typename Callable>
        void cullBillboards(utils::FrustumPlanes const & frustumPlanes,
                            Callable callable) const
        {
            // perform culling
            assert(_spatialIndex);
            _billboardCullBuffer.clear();
            _spatialIndex->cull(frustumPlanes, kBillboardLayer, _billboardCullBuffer);
            _billboardWideCullBuffer.reserve(_billboardCullBuffer.size());
            glm::vec3 const forwardVector = _viewpoint.forwardVector();
            for(void * opaque : _billboardCullBuffer)
            {
                if (BillboardLeaf * billboardLeaf = static_cast<BillboardLeaf *>(opaque);
                    billboardLeaf && billboardLeaf->visible())
                {
                    auto parent = billboardLeaf->_parent.lock();
                    MINIRE_INVARIANT(parent, "a billboard doesn't have a parent");
                    if (parent->_effectiveVisible &&
                        utils::cullingTest(billboardLeaf->worldAabb(), frustumPlanes))
                    {
                        assert(parent->hasGlobalTransform());
                        assert(billboardLeaf->_billboard);
                        float const distToCam = glm::dot(parent->_globalPosition - _viewpoint.position(),
                                                         forwardVector);
                        _billboardWideCullBuffer.emplace_back(distToCam, parent, billboardLeaf);
                    }
                }
            }

            // sort by distance to a camera
            std::ranges::sort(_billboardWideCullBuffer,
                [](BillboardElement const & lhs, BillboardElement const & rhs)
                {
                    BillboardLeaf const * left = std::get<2>(lhs);
                    BillboardLeaf const * right = std::get<2>(rhs);
                    return std::tie(std::get<0>(rhs), left->_zOrder, left)
                         < std::tie(std::get<0>(lhs), right->_zOrder, right);
                });

            // issue callbacks
            for(BillboardElement const & element : _billboardWideCullBuffer)
            {
                auto const & parent = std::get<1>(element);
                auto const & billboardLeaf = std::get<2>(element);
                assert(parent);
                assert(billboardLeaf);
                callable(*billboardLeaf->_billboard,
                         parent->_globalTransform,
                         billboardLeaf->opbId());
            }
        }

        template<typename Callable>
        size_t cullDirectionalLights(size_t limit, Callable callable) const
        {
            size_t index = 0;
            auto it = _directionalLightLeaves.begin();
            while(it != _directionalLightLeaves.end() && index < limit)
            {
                if (auto const & directionalLight = it->lock(); directionalLight)
                {
                    if (directionalLight->visible())
                    {
                        auto parent = directionalLight->_parent.lock();
                        MINIRE_INVARIANT(parent, "a point light doesn't have a parent");
                        if (parent->_effectiveVisible)
                        {
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
            size_t index = 0;
            auto it = _pointLightLeaves.begin();
            while(it != _pointLightLeaves.end() && index < limit)
            {
                if (auto const & pointLight = it->lock(); pointLight)
                {
                    if (pointLight->visible())
                    {
                        auto parent = pointLight->_parent.lock();
                        MINIRE_INVARIANT(parent, "a point light doesn't have a parent");
                        if (parent->_effectiveVisible)
                        {
                            assert(parent->hasGlobalTransform());
                            auto const & current = pointLight->current();
                            callable(index,
                                     parent->_globalPosition,
                                     current._color,
                                     current._attenuation,
                                     current._shadowParams);
                            ++index;
                        }
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
        class OpbIdHolder;

        template<typename Derived,
                 typename ObjectType>
        class Leaf
            : public ObjectType
            , public std::enable_shared_from_this<Derived>
        {
        public:
            using Sptr = std::shared_ptr<Derived>;
            using Wptr = std::weak_ptr<Derived>;

            using ObjectType::name;

            explicit Leaf(std::string name,
                          typename ObjectType::ModelType const & model,
                          std::weak_ptr<Node> parent,
                          SceneImpl & scene)
                : ObjectType(std::move(name), model)
                , _parent(parent)
                , _scene(scene)
            {}

            scene::Node::Wptr parent() const override { return _parent; }
            void setParent(scene::Node::Sptr const & newParent) override;
            models::ScenePath absPath() const override;

        private:
            using ObjectType::propagate;

            void propagate(ObjectType::Mask) override;
            void invalidateParent(ObjectType::Mask);

        private:
            std::weak_ptr<Node> _parent;
            SceneImpl         & _scene;

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
                              std::shared_ptr<rasterizer::Mesh> const & mesh,
                              SceneImpl & scene);

            bool lerp(float, size_t) { return false; } // just for compatibility

            bool isLerpable(size_t) const { return false; }

            SceneImpl::OpbId opbId() const;

            // TODO: lerpable _emissiveFactor

            void revalidate(Mask = kAllFlags) override;

            utils::Aabb const & worldAabb() const { return _worldAabb; }

            void onParentTransformChanged(glm::mat4 const & globalTransform);

        private:
            struct SkinBone
            {
                glm::mat4 const     _inverseBindMatrix;
                std::weak_ptr<Node> _node;
            };
            using SkinBones = std::vector<SkinBone>;

            std::shared_ptr<rasterizer::Mesh> _mesh;
            SkinBones                         _skinBones;
            std::weak_ptr<Node>               _skinOrigin;
            std::unique_ptr<OpbIdHolder>      _opbId;
            utils::Aabb                       _worldAabb;
            scene::SpatialHandler             _spatialHandler;

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
                : Leaf<Derived, SceneType>(std::move(name), model, parent, scene)
                , utils::Lerpable<ModelType>(model)
            {
                // calling at the end, to avoid unwanted calls to virtual methods
                SceneType::propagate(); // must be called before "setAllowPropagation" !
                SceneType::setAllowPropagation(true);
            }
        };

        class DirectionalLightLeaf final
            : public LerpableLeaf<DirectionalLightLeaf,
                                  scene::DirectionalLight,
                                  models::DirectionalLight>
        {
        public:
            using LerpableLeaf::LerpableLeaf;

            void revalidate(Mask = kAllFlags) override;
        };

        class PointLightLeaf final
            : public LerpableLeaf<PointLightLeaf,
                                  scene::PointLight,
                                  models::PointLight>
        {
        public:
            using LerpableLeaf::LerpableLeaf;

            void revalidate(Mask = kAllFlags) override;
        };

        class PerspectiveCameraLeaf final
            : public LerpableLeaf<PerspectiveCameraLeaf,
                                  scene::PerspectiveCamera,
                                  models::PerspectiveCamera>
        {
        public:
            using LerpableLeaf::LerpableLeaf;

            void activate() override;
            void revalidate(Mask = kAllFlags) override;
        };

        class OrthographicCameraLeaf final
            : public LerpableLeaf<OrthographicCameraLeaf,
                                  scene::OrthographicCamera,
                                  models::OrthographicCamera>
        {
        public:
            using LerpableLeaf::LerpableLeaf;

            void activate() override;
            void revalidate(Mask = kAllFlags) override;
        };

        class BillboardLeaf final
            : public Leaf<BillboardLeaf, scene::Billboard>
        {
        public:
            explicit BillboardLeaf(std::string name,
                                   models::Billboard model,
                                   std::weak_ptr<Node> parent,
                                   std::shared_ptr<rasterizer::Billboard> const & billboard,
                                   SceneImpl & scene);

            bool lerp(float, size_t) { return false; } // just for compatibility

            bool isLerpable(size_t) const { return false; }

            SceneImpl::OpbId opbId() const;

            auto const & billboard() const { return _billboard; }

            void revalidate(Mask = kAllFlags) override;

            utils::Aabb const & worldAabb() const { return _worldAabb; }

            void onParentTransformChanged(glm::mat4 const & globalTransform);

        private:
            std::shared_ptr<rasterizer::Billboard> _billboard;
            std::unique_ptr<OpbIdHolder>           _opbId;
            size_t const                           _zOrder;

            utils::Aabb                            _worldAabb;
            scene::SpatialHandler                  _spatialHandler;

            friend class SceneImpl;
        };

        void setActiveCamera(PerspectiveCameraLeaf & camera);
        void setActiveCamera(OrthographicCameraLeaf & camera);

    private:
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
        class ActiveAnimation
            : public scene::Node::PlaybackController
        {
        public:
            using Uptr = std::unique_ptr<ActiveAnimation>;
            using Sequencer = utils::Sequencer<float>;

            ActiveAnimation(AnimationTracksSptr const &,
                            size_t const repeats,
                            float const speedScale);

        public:
            Status status() const override;
            void setPaused(bool) override;

        private:
            struct SequencerSet
            {
                Sequencer::CSptr _translation;
                Sequencer::CSptr _rotation;
                Sequencer::CSptr _scale;
            };

            AnimationTracksSptr          _animationTracks;
            std::vector<SequencerSet>    _animationSequencers;
            std::vector<Sequencer::Sptr> _uniqueSequencers;
            bool                         _paused = false;

            friend class Node;
        };

        class PlaybackStackImpl
            : public scene::Node::PlaybackStack
        {
        public:
            explicit PlaybackStackImpl(Node & node);

            void push(models::AnimationId const &,
                      size_t repeats = 1,
                      float speedScale = 1.0f) override;

            void push(models::AnimationTracks animationTracks,
                      size_t repeats = 1,
                      float speedScale = 1.0f) override;

            void pop() override;
            void clear() override { _activeAnimations.clear(); }

            scene::Node::PlaybackController * top() const override;
            scene::Node::PlaybackController * bottom() const override;

            size_t size() const override { return _activeAnimations.size(); }

            ActiveAnimation * activeAnimation() const;

        private:
            Node                             & _node;
            std::vector<ActiveAnimation::Uptr> _activeAnimations;
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

            PlaybackStack & playbackStack() override { return _playbackStack; }
            PlaybackStack const & playbackStack() const override { return _playbackStack; }

        public:
            size_t size() const override { return _children.size(); }
            bool empty() const override { return _children.empty(); }

            scene::Node::Wptr parent() const override { return _parent; }
            void setParent(scene::Node::Sptr const & newParent) override;
            models::ScenePath absPath() const override;

            void erase(models::ScenePath const &) override;
            void clear() override;

            std::vector<scene::SceneItem> children() const override;

        private:
            scene::SceneItem find(models::ScenePath const &) const override;

        private:
            bool lerp(float weight, size_t epochNumber);

            bool hasGlobalTransform() const
            {
                return !invalidatedAny(kLocalTransformDirty |
                                       kGlobalTransformGray |
                                       kParentTransformChanged);
            }

            using scene::Node::propagate;

            void revalidate(Mask = kAllFlags) override;
            void propagate(Object::Mask) override;
            void invalidateParent(Mask);

            template<typename T>
            void invalidateChildren(Mask);

            template<typename T>
            void notifyLeavesTransformChanged(glm::mat4 const &);

            bool advanceAnimation();

            AnimationTracksSptr
            instantiateTracks(models::AnimationTracks const &) const;

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

            // set if a Node has some Leaf or inner Node which can be lerped
            static constexpr Mask kHasActivateChildren         = mkMask(kFlagsCount + 0);

            // some Leaves or Nodes has values to be revalidated at the new epoch
            static constexpr Mask kHasPendedActivation         = mkMask(kFlagsCount + 1);

            // effective visibility of a node should be re-evaluated
            // due to change of some of parent's effective visibility
            static constexpr Mask kParentVisibilityInvalidated = mkMask(kFlagsCount + 2);

            // recalc visibility of some of nested nodes
            static constexpr Mask kChildVisibilityInvalidated  = mkMask(kFlagsCount + 3);

            // own transform is outdated
            static constexpr Mask kLocalTransformDirty         = mkMask(kFlagsCount + 4);

            // own transform is outdated
            static constexpr Mask kParentTransformChanged      = mkMask(kFlagsCount + 5);

            // own transform is clean, but some children are outdated
            static constexpr Mask kGlobalTransformGray         = mkMask(kFlagsCount + 6);

            // node itself or some of its children (maybe nested) has an active animation
            static constexpr Mask kAnimation                   = mkMask(kFlagsCount + 7);

            // recalc outline of some of nested nodes
            static constexpr Mask kChildOutlineInvalidated     = mkMask(kFlagsCount + 8);

            // effective outline of a node should be re-evaluated
            // due to change of some of parent's effective outline
            static constexpr Mask kParentOutlineInvalidated    = mkMask(kFlagsCount + 9);

            SceneImpl           & _scene;
            LerpableTransform     _localTransform;
            glm::vec3             _globalPosition;
            glm::mat4             _globalTransform;
            glm::mat4             _localTransformMatrix;
            Wptr                  _parent;
            ChildrenMap           _children;
            AnimationSet          _animationSet;
            PlaybackStackImpl     _playbackStack;
            models::Outline       _effectiveOutline = std::monostate();
            bool                  _effectiveVisible = true;

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
            friend class PlaybackStackImpl;
        };

    private:
        // TODO: avoid Wptr in critical loop,
        //       maybe use trivial implementation of SpatialIndex (something like so)

        using ActiveCamera = std::variant<std::monostate,
                                          PerspectiveCameraLeaf::Wptr,
                                          OrthographicCameraLeaf::Wptr>;

        using DirectionalLightLeaves = std::list<DirectionalLightLeaf::Wptr>;
        using PointLightLeaves = std::list<PointLightLeaf::Wptr>;

        using WeakSceneItem = std::variant<std::monostate,
                                           MeshLeaf::Wptr,
                                           BillboardLeaf::Wptr>;

        // Object Picking Buffer
        using OpbIdsSet = std::unordered_set<OpbId>; // TODO: consider std::hive
        using OpbIdToSceneItem = std::unordered_map<OpbId, WeakSceneItem>;

        using PixelEdgeOutlines = std::unordered_map<OpbId, models::outline::PixelEdge>;

        using BillboardElement = std::tuple<float /* dist */,
                                            std::shared_ptr<Node>, /* locked parent */
                                            BillboardLeaf *>;
        using BillboardElements = std::vector<BillboardElement>;

    private:
        void revalidate(Node *, Node::Mask);
        void actualizeViewpoint();
        material::SkinningVectorSptr makeSkinningVector(MeshLeaf const &) const;

        template<typename ItemType>
        static void setParent(ItemType &, scene::Node::Sptr const &);

        OpbId allocateOpbId();
        void releaseOpbId(OpbId);

        scene::SceneItem fetchSceneItem(OpbId const) const;

        // TODO: lerpable _ambientLight

    private:
        Rasterizer                   & _rasterizer;

        // NOTE: while destructing, Leaves will release SpatialHandler
        //       which reference to _spatialIndex, therefore,
        //       it must be destroyed the last one.
        scene::SpatialIndex::Uptr      _spatialIndex;

        // NOTE: while destructing, Leaves will release OpbId's,
        //       thereofe _vacantOpbIds and _opbIdToSceneItem must
        //       be destroyed last.
        //       So the order of members declaration is vital here.
        bool const                     _enableOpb;
        OpbIdsSet                      _vacantOpbIds;
        OpbIdToSceneItem               _opbIdToSceneItem;
        OpbId                          _maxOpbId = 1;

        Node::Sptr                     _root;
        ActiveCamera                   _activeCamera;
        scene::Viewpoint               _viewpoint;
        size_t                         _epochNumber = 0;
        double                         _lerpWeight = 0;
        double                         _frameTime = 0;
        size_t                         _nodesEstimate = 1;

        mutable PixelEdgeOutlines      _pixelEdgeOutlines;

        mutable std::vector<void *>    _meshCullBuffer;
        mutable std::vector<void *>    _billboardCullBuffer;
        mutable DirectionalLightLeaves _directionalLightLeaves;
        mutable PointLightLeaves       _pointLightLeaves;
        mutable BillboardElements      _billboardWideCullBuffer;

        friend class Node;
        friend class MeshLeaf;
        friend class OpbIdHolder;
    };
}
