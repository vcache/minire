#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/grips/orbiting.hpp>
#include <minire/grips/panning.hpp>
#include <minire/logging.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/mesh.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/transform.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    static size_t constexpr kCtrlFps = 30;

    // TODO: This code is duplicated in other examples,
    //       consider to make a base controller w/ orbiting, panning and dual-camera capabilities.

    class CameraSwitch
        : public minire::BasicController
    {
    public:
        explicit CameraSwitch(minire::content::Manager & contentManager)
            : BasicController(contentManager, kCtrlFps)
            , _target(0.0f, 0.0f, 0.0f)
            , _orbiting(_target, 10)
            , _orthographicCamera{._xMag = _orbiting.distance() / 2,
                                  ._yMag = _orbiting.distance() / 2,
                                  ._zNear = 0.001f,
                                  ._zFar = 1000.0f}
            , _perspectiveCamera{._yFov = glm::radians(45.0f),
                                 ._zNear = 0.001f,
                                 ._zFar = 1000.0f,
                                 ._aspectRatio = std::nullopt}
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

            enqueue<SceneNewNode>("light-node", ScenePath(), Transform(glm::vec3(2.0f,  2.0f, 2.0f)), true);
            enqueue<SceneNewPointLight>("bulb", ScenePath{"light-node"}, PointLight(glm::vec4(1, 1, 1, 500), 2), true);

            enqueue<SceneNewNode>("cube-node", ScenePath(), Transform(), true);
            enqueue<SceneNewMesh>("cube", ScenePath{"cube-node"},
                Mesh
                {
                    ._source = mkPath("Box.glb", path::Special::kMeshes, path::Index(0)),
                    ._defaultMaterial = nullptr,
                },
                true);
        }

        void handle(minire::events::application::OnMouseMove const & event) override
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
        }

        void handle(minire::events::application::OnMouseDown const & e) override
        {
            if (e._mouseButton == minire::models::MouseButton::kRight)
            {
                _panning.start(e._x, e._y);
            }
        }

        void handle(minire::events::application::OnMouseUp const &) override
        {
            if (_panning)
            {
                _panning.finish(_target);
            }
        }

        void handle(minire::events::application::OnMouseWheel const & event) override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            _orbiting.updateDistance(-0.5f * event._dy);
            _orbiting.evaluate(_cameraTransform);
            enqueue<SceneSetTransform>(ScenePath{"cam-node"}, _cameraTransform);

            _orthographicCamera._xMag = _orthographicCamera._yMag = _orbiting.distance() / 2.0f;
            enqueue<SceneSetOrthographicCamera>(ScenePath{"cam-node", "ortho-cam"},
                                                _orthographicCamera);
        }

        void handle(minire::events::application::OnResize const & e) override
        {
            _windowSize.x = static_cast<float>(e._width);
            _windowSize.y = static_cast<float>(e._height);
        }

        void handle(minire::events::application::OnKeyDown const & e)
        {
            if (e._key == SDLK_c)
            {
                _isPerspective = !_isPerspective;
                MINIRE_INFO("Camera is switched to {}", _isPerspective ? "PERSPECTIVE"
                                                                       : "ORTHOGRAPHIC");
                using namespace minire::events::controller;
                using namespace minire::models;
                enqueue<SceneActivateCamera>(ScenePath{"cam-node", _isPerspective ? "persp-cam" : "ortho-cam"});
            }
        }

    private:
        glm::vec3                          _target;
        minire::grips::Orbiting            _orbiting;
        minire::models::OrthographicCamera _orthographicCamera;
        minire::models::PerspectiveCamera  _perspectiveCamera;
        minire::models::Transform          _cameraTransform;
        minire::grips::Panning<false>      _panning;
        glm::vec2                          _windowSize;
        bool                               _isPerspective;
    };
}

int main(int, char **)
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);

        // Setup content manager
        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

        // Create and run the Application and its Controller
        minire::Application application(1280, 720, "Camera wwitch", manager);
        application.setController<CameraSwitch>();
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
