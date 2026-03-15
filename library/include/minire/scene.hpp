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
        class Node;

        class Mesh
            : public utils::Object<Mesh, models::Mesh, true /* immutable model */> 
        {
        protected:
            static constexpr size_t kEmmisiveFactor = mkMask(0);
            static constexpr size_t kVisible        = mkMask(1);

        public:
            using Object::Object;

            // TODO: maybe allow mutation of _source/_defaultMaterial?

            glm::vec3 const & emissiveFactor() const { return _emissiveFactor; }
            void setEmissiveFactor(glm::vec3 const & emissiveFactor)
            {
                _emissiveFactor = emissiveFactor;
                invalidate(kEmmisiveFactor);
            }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible) { model(kVisible)._visible = visible; }

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;

        private:
            glm::vec3 _emissiveFactor = glm::vec3(0);
        };

        class DirectionalLight
            : public utils::Object<DirectionalLight, models::DirectionalLight>
        {
        protected:
            static constexpr size_t kColor        = mkMask(0);
            static constexpr size_t kVisible      = mkMask(1);

        public:
            using Object::Object;

            glm::vec3 const & color() const { return model()._color; }
            void setColor(glm::vec3 const & color) { model(kColor)._color = color; }

            // NOTE: shadowParams are immutable yet
            models::MaybeShadowParams const & shadowParams() const { return model()._shadowParams; }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible) { model(kVisible)._visible = visible; }

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
        };

        class PointLight
            : public utils::Object<PointLight, models::PointLight>
        {
        protected:
            static constexpr size_t kColor        = mkMask(0);
            static constexpr size_t kAttenuation  = mkMask(1);
            static constexpr size_t kVisible      = mkMask(2);

        public:
            using Object::Object;

            glm::vec4 const & color() const { return model()._color; }
            void setColor(glm::vec4 const & color) { model(kColor)._color = color; }

            glm::vec4 const & attenuation() const { return model()._attenuation; }
            void setAttenuation(glm::vec4 const & attenuation)
            {
                model(kAttenuation)._attenuation = attenuation;
            }

            // NOTE: shadowParams are immutable yet
            models::MaybeShadowParams const & shadowParams() const { return model()._shadowParams; }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible) { model(kVisible)._visible = visible; }

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
        };

        class PerspectiveCamera
            : public utils::Object<PerspectiveCamera, models::PerspectiveCamera>
        {
        protected:
            static constexpr size_t kYFov        = mkMask(0);
            static constexpr size_t kZNear       = mkMask(1);
            static constexpr size_t kZFar        = mkMask(2);
            static constexpr size_t kAspectRatio = mkMask(3);
            static constexpr size_t kVisible     = mkMask(4);

        public:
            using Object::Object;

            float yFov() const { return model()._yFov; }
            void setYFov(float const yFov) { model(kYFov)._yFov = yFov; }

            float zNear() const { return model()._zNear; }
            void setZNear(float const zNear) { model(kZNear)._zNear = zNear; }

            std::optional<float> zFar() const { return model()._zFar; }
            void setZFar(std::optional<float> const zFar) { model(kZFar)._zFar = zFar; }

            std::optional<float> aspectRatio() const { return model()._aspectRatio; }
            void setAspectRatio(std::optional<float> const aspectRatio)
            {
                model(kAspectRatio)._aspectRatio = aspectRatio;
            }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible) { model(kVisible)._visible = visible; }

            virtual void activate() = 0;

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
        };

        class OrthographicCamera
            : public utils::Object<OrthographicCamera, models::OrthographicCamera>
        {
        protected:
            static constexpr size_t kXMag    = mkMask(0);
            static constexpr size_t kYMag    = mkMask(1);
            static constexpr size_t kZNear   = mkMask(2);
            static constexpr size_t kZFar    = mkMask(3);
            static constexpr size_t kVisible = mkMask(4);

        public:
            using Object::Object;

            float xMag() const { return model()._xMag; }
            void setXMag(float const xMag) { model(kXMag)._xMag = xMag; }

            float yMag() const { return model()._yMag; }
            void setYMag(float const yMag) { model(kYMag)._yMag = yMag; }

            float zNear() const { return model()._zNear; }
            void setZNear(float const zNear) { model(kZNear)._zNear = zNear; }

            float zFar() const { return model()._zFar; }
            void setZFar(float const zFar) { model(kZFar)._zFar = zFar; }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible) { model(kVisible)._visible = visible; }

            virtual void activate() = 0;

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
        };

        class Billboard
            : public utils::Object<Billboard, models::Billboard>
        {
        protected:
            static constexpr size_t kVisible = mkMask(1);

        public:
            using Object::Object;

            // TODO: erase these methods since they are immutable for now
            models::Billboard::Content const & content() const { return model()._content; }

            models::Billboard::Placement const & placement() const { return model()._placement; }

            // TODO: zOrder isnt't model data, they are more like instance data (as Node/Leaf in Scene) (like Visible)
            size_t zOrder() const { return model()._zOrder; }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible) { model(kVisible)._visible = visible; }

            virtual std::weak_ptr<Node> parent() const = 0;
            virtual void setParent(std::shared_ptr<Node> const & newParent) = 0;
        };

        class Node
            : public utils::Object<Node, models::Node>
        {
        protected:
            static constexpr size_t kOrigin  = mkMask(0);
            static constexpr size_t kVisible = mkMask(1);

            using SceneItem = std::variant<std::monostate,
                                           std::shared_ptr<Mesh>,
                                           std::shared_ptr<DirectionalLight>,
                                           std::shared_ptr<PointLight>,
                                           std::shared_ptr<PerspectiveCamera>,
                                           std::shared_ptr<OrthographicCamera>,
                                           std::shared_ptr<Billboard>,
                                           std::shared_ptr<Node>>;
            virtual SceneItem find(models::ScenePath const &) const = 0;

        public:
            using Object::Object;

            models::Transform const & origin() const { return model()._origin; }
            models::Transform & origin() { return model(kOrigin)._origin; }
            void setOrigin(models::Transform origin) { model(kOrigin)._origin = std::move(origin); }

            bool visible() const { return model()._visible; }
            void setVisible(bool visible) { model(kVisible)._visible = visible; }

        public:
            // TODO: make consistent w/ Sprites and Labels
            virtual Node::Sptr make(std::string const & name, models::Node) = 0;
            virtual Mesh::Sptr make(std::string const & name, models::Mesh) = 0;
            virtual DirectionalLight::Sptr make(std::string const &, models::DirectionalLight) = 0;
            virtual PointLight::Sptr make(std::string const &, models::PointLight) = 0;
            virtual PerspectiveCamera::Sptr make(std::string const &, models::PerspectiveCamera) = 0;
            virtual OrthographicCamera::Sptr make(std::string const &, models::OrthographicCamera) = 0;
            virtual Billboard::Sptr make(std::string const & name, models::Billboard) = 0;

            Node::Sptr make(models::Node model) { return make(utils::newUuid(), std::move(model)); }
            Mesh::Sptr make(models::Mesh model) { return make(utils::newUuid(), std::move(model)); }
            DirectionalLight::Sptr make(models::DirectionalLight model) { return make(utils::newUuid(), std::move(model)); }
            PointLight::Sptr make(models::PointLight model) { return make(utils::newUuid(), std::move(model)); }
            PerspectiveCamera::Sptr make(models::PerspectiveCamera model) { return make(utils::newUuid(), std::move(model)); }
            OrthographicCamera::Sptr make(models::OrthographicCamera model) { return make(utils::newUuid(), std::move(model)); }
            Billboard::Sptr make(models::Billboard model) { return make(utils::newUuid(), std::move(model)); }

            virtual void makeFromSource(content::Path const &, content::Manager &, bool visible) = 0;

        public:
            static constexpr size_t kInfinitely = std::numeric_limits<size_t>::max();

            // NOTE: will replace any existing animations, i.e.
            //       this command has "assign" semantics.
            virtual void makeAnimationSet(models::AnimationSet animationSet) = 0;

            /**
             * \param repeats kInfinitely or a specific value,
             *                for example, 1 for a single repeat
             * \param speedScale 1.0 for a normal speed,
             *                   less than one = slowdown
             *                   more than one = speedup
             * */
            virtual void playAnimation(models::AnimationId const &,
                                       size_t repeats = 1,
                                       float speedScale = 1.0f) = 0;

            virtual void stopAnimation() = 0;

            // Params semantics as in makeAnimationSet and playAnimation
            virtual void inlineAnimation(models::AnimationTracks animationTracks,
                                         size_t repeats = 1, // or kInfinitely
                                         float speedScale = 1.0f) = 0;

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

            virtual void dispose(models::ScenePath const &) = 0;
            virtual void disposeAll() = 0;

            template<typename... Path>
            void dispose(Path && ... path) requires kNotScenePath<Path...>
            {
                dispose(models::mkScenePath(std::forward<Path>(path)...));
            }
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