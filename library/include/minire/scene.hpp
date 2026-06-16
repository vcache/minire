#pragma once

#include <minire/content/id.hpp>
#include <minire/content/path.hpp>
#include <minire/models/animations.hpp>
#include <minire/models/billboard.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/directional-light.hpp>
#include <minire/models/mesh.hpp>
#include <minire/models/node.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/scene-path.hpp>
#include <minire/models/shadow-params.hpp>
#include <minire/models/transform.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/object.hpp>
#include <minire/utils/user-data.hpp>
#include <minire/utils/uuid.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <variant>

namespace minire::content { class Manager; }

namespace minire
{
    // TODO: base model for leaves:
    // ScreenModel { bool  _visible }
    // ScreenModel2D : ScreenModel { zOrder }
    // ScreenModel3D : ScreenModel { ... }

    // TODO: re-consider move/copy semantics, maybe pass data by a const-ref instead a value
    // TODO: split into separate files

    template<typename... Path>
    concept kNotScenePath = (!std::same_as<models::ScenePath, std::decay_t<Path>> && ...);

    namespace scene
    {
        class Mesh;
        class DirectionalLight;
        class PointLight;
        class PerspectiveCamera;
        class OrthographicCamera;
        class Billboard;
        class Node;

        using SceneItem = std::variant<std::monostate,
                                       std::shared_ptr<Mesh>,
                                       std::shared_ptr<DirectionalLight>,
                                       std::shared_ptr<PointLight>,
                                       std::shared_ptr<PerspectiveCamera>,
                                       std::shared_ptr<OrthographicCamera>,
                                       std::shared_ptr<Billboard>,
                                       std::shared_ptr<Node>>;

        class Mesh
            : public utils::Object<Mesh, models::Mesh, true /* immutable model */>
            , public utils::UserData
        {
        protected:
            static constexpr Mask kVisible        = mkMask(0);
            static constexpr Mask kOutline        = mkMask(1);
            static constexpr Mask kEmmisiveFactor = mkMask(2);
            static constexpr Mask kEnableOpb      = mkMask(3);

            static constexpr Mask kBaseMask = kVisible
                                            | kOutline
                                            | kEmmisiveFactor
                                            | kEnableOpb;

            static constexpr size_t kFlagsCount = 4; // an offset for descendants

        public:
            using Object::Object;

            // TODO: maybe allow mutation of _source/_defaultMaterial?

            bool visible() const { return model()._visible; }
            void setVisible(bool visible)
            {
                if (this->visible() != visible)
                {
                    model(kVisible)._visible = visible;
                }
            }

            models::Outline const & outline() const { return model()._outline; }
            void setOutline(models::Outline const & outline)
            {
                if (this->outline() != outline)
                {
                    model(kOutline)._outline = outline;
                }
            }

            glm::vec3 const & emissiveFactor() const { return model()._emissiveFactor; }
            void setEmissiveFactor(glm::vec3 const & emissiveFactor)
            {
                if (this->emissiveFactor() != emissiveFactor)
                {
                    model(kEmmisiveFactor)._emissiveFactor = emissiveFactor;
                }
            }

            bool enableOpb() const { return model()._enableOpb; }
            void setEnableOpb(bool enableOpb)
            {
                if (this->enableOpb() != enableOpb)
                {
                    model(kEnableOpb)._enableOpb = enableOpb;
                }
            }

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
            virtual models::ScenePath absPath() const = 0;

            void detach() { setParent({}); }
        };

        class DirectionalLight
            : public utils::Object<DirectionalLight, models::DirectionalLight>
            , public utils::UserData
        {
        protected:
            static constexpr Mask kColor        = mkMask(0);
            static constexpr Mask kVisible      = mkMask(1);

            static constexpr Mask kBaseMask = kColor
                                            | kVisible;

            static constexpr size_t kFlagsCount = 2; // an offset for descendants

        public:
            using Object::Object;

            glm::vec3 const & color() const { return model()._color; }
            void setColor(glm::vec3 const & color)
            {
                if (this->color() != color)
                {
                    model(kColor)._color = color;
                }
            }

            // NOTE: shadowParams are immutable yet
            models::MaybeShadowParams const & shadowParams() const { return model()._shadowParams; }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible)
            {
                if (this->visible() != visible)
                {
                    model(kVisible)._visible = visible;
                }
            }

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
            virtual models::ScenePath absPath() const = 0;

            void detach() { setParent({}); }
        };

        class PointLight
            : public utils::Object<PointLight, models::PointLight>
            , public utils::UserData
        {
        protected:
            static constexpr Mask kColor        = mkMask(0);
            static constexpr Mask kAttenuation  = mkMask(1);
            static constexpr Mask kVisible      = mkMask(2);

            static constexpr Mask kBaseMask = kColor
                                            | kAttenuation
                                            | kVisible;

            static constexpr size_t kFlagsCount = 3; // an offset for descendants

        public:
            using Object::Object;

            glm::vec4 const & color() const { return model()._color; }
            void setColor(glm::vec4 const & color)
            {
                if (this->color() != color)
                {
                    model(kColor)._color = color;
                }
            }

            glm::vec4 const & attenuation() const { return model()._attenuation; }
            void setAttenuation(glm::vec4 const & attenuation)
            {
                if (this->attenuation() != attenuation)
                {
                    model(kAttenuation)._attenuation = attenuation;
                }
            }

            // NOTE: shadowParams are immutable yet
            models::MaybeShadowParams const & shadowParams() const { return model()._shadowParams; }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible)
            {
                if (this->visible() != visible)
                {
                    model(kVisible)._visible = visible;
                }
            }

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
            virtual models::ScenePath absPath() const = 0;

            void detach() { setParent({}); }
        };

        class PerspectiveCamera
            : public utils::Object<PerspectiveCamera, models::PerspectiveCamera>
            , public utils::UserData
        {
        protected:
            static constexpr Mask kYFov        = mkMask(0);
            static constexpr Mask kZNear       = mkMask(1);
            static constexpr Mask kZFar        = mkMask(2);
            static constexpr Mask kAspectRatio = mkMask(3);
            static constexpr Mask kVisible     = mkMask(4);

            static constexpr Mask kBaseMask = kYFov | kZNear | kZFar
                                            | kAspectRatio | kVisible;

            static constexpr size_t kFlagsCount = 5; // an offset for descendants

        public:
            using Object::Object;

            float yFov() const { return model()._yFov; }
            void setYFov(float const yFov)
            {
                if (this->yFov() != yFov)
                {
                    model(kYFov)._yFov = yFov;
                }
            }

            float zNear() const { return model()._zNear; }
            void setZNear(float const zNear)
            {
                if (this->zNear() != zNear)
                {
                    model(kZNear)._zNear = zNear;
                }
            }

            std::optional<float> zFar() const { return model()._zFar; }
            void setZFar(std::optional<float> const zFar)
            {
                if (this->zFar() != zFar)
                {
                    model(kZFar)._zFar = zFar;
                }
            }

            std::optional<float> aspectRatio() const { return model()._aspectRatio; }
            void setAspectRatio(std::optional<float> const aspectRatio)
            {
                if (this->aspectRatio() != aspectRatio)
                {
                    model(kAspectRatio)._aspectRatio = aspectRatio;
                }
            }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible)
            {
                if (this->visible() != visible)
                {
                    model(kVisible)._visible = visible;
                }
            }

            virtual void activate() = 0;

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
            virtual models::ScenePath absPath() const = 0;

            void detach() { setParent({}); }
        };

        class OrthographicCamera
            : public utils::Object<OrthographicCamera, models::OrthographicCamera>
            , public utils::UserData
        {
        protected:
            static constexpr Mask kXMag    = mkMask(0);
            static constexpr Mask kYMag    = mkMask(1);
            static constexpr Mask kZNear   = mkMask(2);
            static constexpr Mask kZFar    = mkMask(3);
            static constexpr Mask kVisible = mkMask(4);

            static constexpr Mask kBaseMask = kXMag | kYMag | kZNear | kZFar
                                            | kVisible;

            static constexpr size_t kFlagsCount = 5; // an offset for descendants

        public:
            using Object::Object;

            float xMag() const { return model()._xMag; }
            void setXMag(float const xMag)
            {
                if (this->xMag() != xMag)
                {
                    model(kXMag)._xMag = xMag;
                }
            }

            float yMag() const { return model()._yMag; }
            void setYMag(float const yMag)
            {
                if (this->yMag() != yMag)
                {
                    model(kYMag)._yMag = yMag;
                }
            }

            float zNear() const { return model()._zNear; }
            void setZNear(float const zNear)
            {
                if (this->zNear() != zNear)
                {
                    model(kZNear)._zNear = zNear;
                }
            }

            float zFar() const { return model()._zFar; }
            void setZFar(float const zFar)
            {
                if (this->zFar() != zFar)
                {
                    model(kZFar)._zFar = zFar;
                }
            }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible)
            {
                if (this->visible() != visible)
                {
                    model(kVisible)._visible = visible;
                }
            }

            virtual void activate() = 0;

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
            virtual models::ScenePath absPath() const = 0;

            void detach() { setParent({}); }
        };

        // TODO: add emissive factor (as in Mesh)
        class Billboard
            : public utils::Object<Billboard, models::Billboard>
            , public utils::UserData
        {
        protected:
            static constexpr Mask kOutline   = mkMask(0);
            static constexpr Mask kEnableOpb = mkMask(1);
            static constexpr Mask kVisible   = mkMask(2);

            static constexpr Mask kBaseMask = kOutline
                                            | kEnableOpb
                                            | kVisible;

            static constexpr size_t kFlagsCount = 3; // an offset for descendants

        public:
            using Object::Object;

            // TODO: erase these methods since they are immutable for now
            models::Billboard::Content const & content() const { return model()._content; }

            models::Billboard::Placement const & placement() const { return model()._placement; }

            // TODO: zOrder isnt't model data, they are more like instance data (as Node/Leaf in Scene) (like Visible)
            size_t zOrder() const { return model()._zOrder; }

            models::Outline const & outline() const { return model()._outline; }
            void setOutline(models::Outline const & outline)
            {
                if (this->outline() != outline)
                {
                    model(kOutline)._outline = outline;
                }
            }

            bool enableOpb() const { return model()._enableOpb; }
            void setEnableOpb(bool enableOpb)
            {
                if (this->enableOpb() != enableOpb)
                {
                    model(kEnableOpb)._enableOpb = enableOpb;
                }
            }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible)
            {
                if (this->visible() != visible)
                {
                    model(kVisible)._visible = visible;
                }
            }

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
            virtual models::ScenePath absPath() const = 0;

            void detach() { setParent({}); }
        };

        // TODO: add en Emmissive Factor (as in Mesh)
        class Node
            : public utils::Object<Node, models::Node>
            , public utils::UserData
        {
        protected:
            static constexpr Mask kOrigin  = mkMask(0);
            static constexpr Mask kOutline = mkMask(1);
            static constexpr Mask kVisible = mkMask(2);

            static constexpr Mask kBaseMask = kOrigin
                                            | kOutline
                                            | kVisible;

            static constexpr size_t kFlagsCount = 3; // an offset for descendants

            virtual SceneItem find(models::ScenePath const &) const = 0;

        public:
            using Object::Object;

            models::Transform const & origin() const { return model()._origin; }
            models::Transform & origin() { return model(kOrigin)._origin; }

            void setOrigin(models::Transform origin)
            {
                if (this->origin() != origin)
                {
                    model(kOrigin)._origin = std::move(origin);
                }
            }

            models::Outline const & outline() const { return model()._outline; }
            void setOutline(models::Outline const & outline)
            {
                if (this->outline() != outline)
                {
                    model(kOutline)._outline = outline;
                }
            }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible)
            {
                if (this->visible() != visible)
                {
                    model(kVisible)._visible = visible;
                }
            }

        public:
            // Generally, user code should store pointers to these objects as Wptr,
            // but it won't hurt storing as Sptr if the user understands possible pitfalls.

            // make -> insert to align w/ stl naming
            virtual Node::Sptr make(std::string const & name, models::Node) = 0;
            virtual Mesh::Sptr make(std::string const & name, models::Mesh) = 0;
            virtual DirectionalLight::Sptr make(std::string const &, models::DirectionalLight) = 0;
            virtual PointLight::Sptr make(std::string const &, models::PointLight) = 0;
            virtual PerspectiveCamera::Sptr make(std::string const &, models::PerspectiveCamera) = 0;
            virtual OrthographicCamera::Sptr make(std::string const &, models::OrthographicCamera) = 0;
            virtual Billboard::Sptr make(std::string const & name, models::Billboard) = 0;

            template<typename T>
            auto make(T model) { return make(utils::newUuid(), std::move(model)); }

            virtual void makeFromSource(content::Path const &, content::Manager &, bool visible) = 0;

        public:
            static constexpr Mask kInfinitely = std::numeric_limits<size_t>::max();

            // NOTE: will replace any existing animations, i.e.
            //       this command has "assign" semantics.
            virtual void makeAnimationSet(models::AnimationSet animationSet) = 0;

            // Animations are stored as a LIFO-queue (a stack).

            class PlaybackController
                : public utils::UserData
            {
            public:
                using Uptr = std::unique_ptr<PlaybackController>;

                PlaybackController() = default;
                virtual ~PlaybackController() = default;

                enum class Status { kActive, kPaused, kFinished, };

                virtual Status status() const = 0;
                virtual void setPaused(bool) = 0;
            };

            class PlaybackStack
            {
            public:
                PlaybackStack() = default;
                virtual ~PlaybackStack() = default;

            public:
                /**
                 * \param repeats kInfinitely or a specific value,
                 *                for example, 1 for a single repeat
                 * \param speedScale 1.0 for a normal speed,
                 *                   less than one = slowdown
                 *                   more than one = speedup
                 * */
                virtual void push(models::AnimationId const &,
                                  size_t repeats = 1,
                                  float speedScale = 1.0f) = 0;

                // Params semantics as in makeAnimationSet and playAnimation
                virtual void push(models::AnimationTracks animationTracks,
                                  size_t repeats = 1, // or kInfinitely
                                  float speedScale = 1.0f) = 0;

                // does nothing when empty
                virtual void pop() = 0;
                virtual void clear() = 0;

                // resturns nullptr whenm emptry,
                // the cakller  MUST NOT  store the pointer for a long time,
                // must not outlive the playback time!
                virtual PlaybackController * top() const = 0;
                virtual PlaybackController * bottom() const = 0;

                virtual size_t size() const = 0;
            };

            virtual PlaybackStack & playbackStack() = 0;
            virtual PlaybackStack const & playbackStack() const = 0;

        public:
            template<typename T>
            typename T::Sptr find(models::ScenePath const & path) const
            {
                SceneItem result = find(path);
                if (std::holds_alternative<std::monostate>(result)) return {};
                MINIRE_INVARIANT(std::holds_alternative<typename T::Sptr>(result),
                                 "expected {} but got {}: {}",
                                 utils::demangle<T>(), result.index(), path);
                return std::get<typename T::Sptr>(result);
            }

            template<typename T, typename... Path>
            typename T::Sptr find(Path && ... path) const requires kNotScenePath<Path...>
            {
                return find<T>(models::mkScenePath(std::forward<Path>(path)...));
            }

            template<typename T>
            T const & at(models::ScenePath const & path) const
            {
                SceneItem result = find(path);
                MINIRE_INVARIANT(!std::holds_alternative<std::monostate>(result),
                                 "no such element: {} in {}", path, name());
                MINIRE_INVARIANT(std::holds_alternative<typename T::Sptr>(result),
                                 "expected {} but got {}: {} in {}",
                                 utils::demangle<T>(), result.index(), path, name());
                typename T::Sptr ptr = std::get<typename T::Sptr>(result);
                MINIRE_INVARIANT(ptr, "bad (empty) ptr: {}", path);
                return *ptr;
            }

            template<typename T, typename... Path>
            T const & at(Path && ... path) const requires kNotScenePath<Path...>
            {
                return at<T>(models::mkScenePath(std::forward<Path>(path)...));
            }

            template<typename T>
            T & at(models::ScenePath const & path)
            {
                return const_cast<T &>(static_cast<Node const *>(this)->at<T>(path));
            }

            template<typename T, typename... Path>
            T & at(Path && ... path) requires kNotScenePath<Path...>
            {
                return at<T>(models::mkScenePath(std::forward<Path>(path)...));
            }

            virtual size_t size() const = 0;
            virtual bool empty() const = 0;

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
            virtual models::ScenePath absPath() const = 0;

            virtual void erase(models::ScenePath const &) = 0;
            virtual void clear() = 0;

            virtual std::vector<SceneItem> children() const = 0;

            template<typename... Path>
            void erase(Path && ... path) requires kNotScenePath<Path...>
            {
                erase(models::mkScenePath(std::forward<Path>(path)...));
            }

            void detach() { setParent({}); }
        };
    }

    class Scene
    {
    public:
        virtual ~Scene() = default;

        virtual scene::Node & root() const = 0;

        // NOTE: pass models::ScenePath{} to unset an active camera
        virtual void setActiveCamera(models::ScenePath const & = {}) = 0;

        template<typename... Path>
        void setActiveCamera(Path && ... path) requires kNotScenePath<Path...>
        {
            setActiveCamera(models::mkScenePath(std::forward<Path>(path)...));
        }

        virtual void reset() = 0;

        // "x" and "y" are screen-space coordinates
        // (same as in OnMouseMove._absX and OnMouseMove._absY)
        // NOTE: it might have significant latency
        // Returns only visible and renderable Scene leaves (Mesh or Billboard).
        virtual scene::SceneItem fetchSceneItem(size_t const x, size_t const y) const = 0;

        // A faster method to fetch a Mesh under the mouse cursor.
        // Returns only visible and renderable Scene leaves (Mesh or Billboard).
        virtual scene::SceneItem fetchHotSceneItem() const = 0;

    public:
        glm::vec3 const & ambientLight() const { return _ambientLight; }
        void setAmbientLight(glm::vec3 const & ambientLight)
        {
            _ambientLight = ambientLight;
        }

    private:
        glm::vec3 _ambientLight = glm::vec3(0.03f);
    };
}
