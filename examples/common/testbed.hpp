#pragma once

#include <minire/application.hpp>
#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/grips/orbiting.hpp>
#include <minire/grips/panning.hpp>
#include <minire/logging.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/font-face.hpp>
#include <minire/models/pbr-material.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/transform.hpp>

#include <glm/gtc/quaternion.hpp> // for quatLookAt

namespace minire::examples
{
    static constexpr auto kFontFace = "ucs-6x13-example";

    class TestbedController
        : public minire::BasicController
    {
        glm::quat lookAt(glm::vec3 lookFrom, glm::vec3 lookTo,
                         glm::vec3 worldUp =  glm::vec3(0.0f, 1.0f, 0.0f))
        {
            // TODO: why lookFrom and lookTo are flipped? Isnt' is should be (lookTo - lookFrom)
            glm::vec3 const direction = glm::normalize(lookFrom - lookTo);
            return glm::quatLookAt(direction, worldUp);
        }

    public:
        explicit TestbedController(minire::content::Manager & contentManager)
            : BasicController(contentManager, 30 /* controller fps */)
            , _target(0.0f, 0.0f, 0.0f)
            , _orbiting(_target, 10)
            , _orthographicCamera{._xMag = _orbiting.distance() / 2,
                                  ._yMag = _orbiting.distance() / 2,
                                  ._zNear = 0.1f,
                                  ._zFar = 100.0f}
            , _perspectiveCamera{._yFov = glm::radians(45.0f),
                                 ._zNear = 0.1f,
                                 ._zFar = 100.0f,
                                 ._aspectRatio = std::nullopt}
            , _isDirectLightEnabled(false)
            , _isPointLightEnabled(true)
            , _isFloorPlaneEnabled(true)
            , _isPerspective(true)
        {}

        void start() override
        {
            using namespace minire::content;
            using namespace minire::events::controller;
            using namespace minire::models;

            _orbiting.evaluate(_cameraTransform);
            enqueue<SceneNewNode>("cam-node", ScenePath(), _cameraTransform, true);
            enqueue<SceneNewPerspectiveCamera>("persp-cam", ScenePath{"cam-node"}, _perspectiveCamera, true);
            enqueue<SceneNewOrthographicCamera>("ortho-cam", ScenePath{"cam-node"}, _orthographicCamera, true);
            enqueue<SceneActivateCamera>(ScenePath{"cam-node", "persp-cam"});

            enqueue<SceneNewNode>("floor-node", ScenePath(), Transform(glm::vec3(0, -.5, 0)), true);
            enqueue<SceneNewMesh>("floor-plane", ScenePath{"floor-node"},
                Mesh
                {
                    ._source = mkPath("../common/floor-plane.glb", path::Special::kMeshes, path::Index(0)),
                    ._defaultMaterial = [this]
                    {
                        auto result = std::make_shared<PbrMaterial>();
                        result->_albedoFactor = glm::vec3(1.0f, 0.0f, 0.0f);
                        result->_metallicFactor = 0.0f;
                        result->_roughnessFactor = 1.0f;
                        return result;
                    }()
                },
                _isFloorPlaneEnabled);


            enqueue<SceneNewNode>("directlight-node", ScenePath(),
                                  Transform(glm::vec3(0), lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0))),
                                  true);
            enqueue<SceneNewDirectionalLight>("sun", ScenePath{"directlight-node"},
                                              DirectionalLight(glm::vec3(0, 10, 0), true), _isDirectLightEnabled);

            enqueue<SceneNewNode>("pointlight-node", ScenePath(),
                                  Transform(glm::vec3(2.0f,  2.0f, 2.0f)), true);
            enqueue<SceneNewPointLight>("bulb", ScenePath{"pointlight-node"},
                                        PointLight(glm::vec4(1, 1, 1, 500), 2), _isPointLightEnabled);
        }

        bool handle(minire::events::application::OnMouseMove const & event) override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            bool updated = false;
            if (event._left)
            {
                _orbiting.updateAngles(0.01f * static_cast<float>(event._relX),
                                       0.01f * static_cast<float>(event._relY));
                updated = true;
            }
            else if (event._right && _panning)
            {
                if (_isPerspective)
                {
                    _orbiting.target() = _panning.update(event._absX, event._absY,
                                                         _cameraTransform.matrix(),
                                                         _target, _perspectiveCamera,
                                                         _windowSize, _cameraTransform._translation);
                }
                else
                {
                    _orbiting.target() = _panning.update(event._absX, event._absY,
                                                         _cameraTransform.matrix(),
                                                         _target, _orthographicCamera,
                                                         _windowSize, _cameraTransform._translation);
                }
                updated = true;
            }

            if (updated)
            {
                _orbiting.evaluate(_cameraTransform);
                enqueue<SceneSetTransform>(ScenePath{"cam-node"}, _cameraTransform);
            }

            return true;
        }

        bool handle(minire::events::application::OnMouseDown const & e) override
        {
            if (e._mouseButton == minire::models::MouseButton::kRight)
            {
                _panning.start(e._x, e._y);
            }

            return true;
        }

        bool handle(minire::events::application::OnMouseUp const &) override
        {
            if (_panning)
            {
                _panning.finish(_target);
            }
            return true;
        }

        bool handle(minire::events::application::OnMouseWheel const & event) override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            _orbiting.updateDistance(-0.5f * event._dy);
            _orbiting.evaluate(_cameraTransform);
            enqueue<SceneSetTransform>(ScenePath{"cam-node"}, _cameraTransform);

            _orthographicCamera._xMag = _orthographicCamera._yMag = _orbiting.distance() / 2.0f;
            enqueue<SceneSetOrthographicCamera>(ScenePath{"cam-node", "ortho-cam"},
                                                _orthographicCamera);
            return true;
        }

        void handle(minire::events::application::OnResize const & e) override
        {
            _windowSize.x = static_cast<float>(e._width);
            _windowSize.y = static_cast<float>(e._height);
        }

        bool handle(minire::events::application::OnKeyDown const & e)
        {
            using namespace minire::events::controller;
            using namespace minire::models;
            switch(e._key)
            {
                case SDLK_c:
                    _isPerspective = !_isPerspective;
                    MINIRE_INFO("Camera is switched to {}", _isPerspective ? "PERSPECTIVE"
                                                                           : "ORTHOGRAPHIC");
                    enqueue<SceneActivateCamera>(ScenePath{"cam-node", _isPerspective ? "persp-cam" : "ortho-cam"});
                    break;

                case SDLK_d:
                    _isDirectLightEnabled = !_isDirectLightEnabled;
                    MINIRE_INFO("Toggle direct light: {}", _isDirectLightEnabled);
                    enqueue<SceneSetVisibility>(ScenePath{"directlight-node", "sun"}, _isDirectLightEnabled);
                    break;

                case SDLK_p:
                    _isPointLightEnabled = !_isPointLightEnabled;
                    MINIRE_INFO("Toggle point light: {}", _isPointLightEnabled);
                    enqueue<SceneSetVisibility>(ScenePath{"pointlight-node", "bulb"}, _isPointLightEnabled);
                    break;

                case SDLK_f:
                    _isFloorPlaneEnabled = !_isFloorPlaneEnabled;
                    MINIRE_INFO("Toggle floor plane: {}", _isFloorPlaneEnabled);
                    enqueue<SceneSetVisibility>(ScenePath{"floor-node", "floor-plane"}, _isFloorPlaneEnabled);
                    break;
            }
            return true;
        }

    private:
        glm::vec3                          _target;
        minire::grips::Orbiting            _orbiting;
        minire::models::OrthographicCamera _orthographicCamera;
        minire::models::PerspectiveCamera  _perspectiveCamera;
        minire::models::Transform          _cameraTransform;
        minire::grips::Panning<false>      _panning;
        glm::vec2                          _windowSize;
        bool                               _isDirectLightEnabled;
        bool                               _isPointLightEnabled;
        bool                               _isFloorPlaneEnabled;
        bool                               _isPerspective;
    };

    template<typename ControllerType>
    int main(std::string const & title)
    {
       try
        {
            // Initialization
            minire::logging::setVerbosity(minire::logging::Level::kDebug);

            // Setup content manager
            minire::content::Manager manager;
            manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

            auto lease = manager.upload(kFontFace, minire::models::FontFace
                {
                    ._regular = "../common/6x13.bdf",
                    ._bold = "../common/6x13B.bdf",
                    ._italic = "../common/6x13O.bdf",
                    ._glyphWidth = 6,
                    ._glyphHeight = 13,
                });

            // Create and run the Application and its Controller
            minire::Application application(1280, 720, title, manager);
            application.setController<ControllerType>();
            application.setVsync(true);
            application.setGlDebug(false);

            // Main loop
            application.run();

            // Finish
            return EXIT_SUCCESS;
        }
        catch(std::exception const & e)
        {
            MINIRE_ERROR("Fatal error:\n{}", e.what());
        }
        catch(...)
        {
            MINIRE_ERROR("Fatal error: (unknown error)");
        }

        return EXIT_FAILURE;
    }
}