#pragma once

#include <minire/application.hpp>
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

#include <cstdlib> // for EXIT_SUCCESS

namespace minire::examples
{
    static constexpr auto kFontFace = "ucs-6x13-example";

    class TestbedApplication
        : public minire::Application
    {
        glm::quat lookAt(glm::vec3 lookFrom, glm::vec3 lookTo,
                         glm::vec3 worldUp =  glm::vec3(0.0f, 1.0f, 0.0f))
        {
            // TODO: why lookFrom and lookTo are flipped? Isnt' is should be (lookTo - lookFrom)
            glm::vec3 const direction = glm::normalize(lookFrom - lookTo);
            return glm::quatLookAt(direction, worldUp);
        }

    public:
        explicit TestbedApplication(int width, int height,
                                    std::string const & title,
                                    content::Manager & contentManager)
            : Application(width, height, title, contentManager)
            , _target(0.0f, 0.0f, 0.0f)
            , _orbiting(_target, 10)
            , _isDirectLightEnabled(false)
            , _isPointLightEnabled(true)
            , _isFloorPlaneEnabled(true)
            , _isPerspective(true)
        {}

        void onStart() override
        {
            Application::onStart();

            using namespace minire::content;
            using namespace minire::models;

            auto & root = scene().root();

            _orbiting.evaluate(_cameraTransform);

            _cameraNode = root.make("cam-node", Node{_cameraTransform, true});
            {
                _perspectiveCamera = _cameraNode->make("persp-cam",
                    PerspectiveCamera
                    {
                        ._yFov = glm::radians(45.0f),
                        ._zNear = 0.1f,
                        ._zFar = 100.0f,
                        ._aspectRatio = std::nullopt,
                        ._visible = true,
                    });

                _orthographicCamera = _cameraNode->make("ortho-cam",
                    OrthographicCamera
                    {   ._xMag = _orbiting.distance() / 2,
                        ._yMag = _orbiting.distance() / 2,
                        ._zNear = 0.1f,
                        ._zFar = 100.0f,
                        ._visible = true,
                    });

                _perspectiveCamera->activate();
            }

            auto floorNode = root.make("floor-node", Node{Transform(glm::vec3(0, -.5, 0)), true});
            floorNode->make("floor-plane",
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
                    }(),
                    ._skin = {},
                    ._visible = _isFloorPlaneEnabled,
                });

            auto directlightNode = root.make("directlight-node",
                Node{Transform(glm::vec3(0), lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0))), true});
            directlightNode->make("sun",
                DirectionalLight(glm::vec3(0, 10, 0), ShadowParams{4096, false}, _isDirectLightEnabled));

            auto pointlightNode = root.make("pointlight-node",
                Node{Transform(glm::vec3(2.0f,  2.0f, 2.0f)), true});
            pointlightNode->make("bulb",
                PointLight(glm::vec4(1, 1, 1, 500), 2, ShadowParams{}, _isPointLightEnabled));
        }

        bool onStep() override
        {
            // Update scene 5 times slower to show how
            // interpolation compensates a gap between
            // a render and a controller timings.

            return 0 == (frame() % 5);
        }

        bool handle(application::OnMouseMove const & e) override
        {
            bool updated = false;
            if (e._left)
            {
                _orbiting.updateAngles(0.01f * static_cast<float>(e._relX),
                                       0.01f * static_cast<float>(e._relY));
                updated = true;
            }
            else if (e._right && _panning)
            {
                if (_isPerspective)
                {
                    assert(_perspectiveCamera);
                    _orbiting.target() = _panning.update(e._absX, e._absY,
                                                         _cameraTransform.matrix(),
                                                         _target, _perspectiveCamera->model(),
                                                         _windowSize, _cameraTransform._translation);
                }
                else
                {
                    assert(_orthographicCamera);
                    _orbiting.target() = _panning.update(e._absX, e._absY,
                                                         _cameraTransform.matrix(),
                                                         _target, _orthographicCamera->model(),
                                                         _windowSize, _cameraTransform._translation);
                }
                updated = true;
            }

            if (updated)
            {
                _orbiting.evaluate(_cameraTransform);

                assert(_cameraNode);
                _cameraNode->setOrigin(_cameraTransform);
            }

            return true;
        }

        bool handle(application::OnMouseDown const & e) override
        {
            if (models::MouseButton::kRight == e._mouseButton)
            {
                _panning.start(e._x, e._y);
            }

            return true;
        }

        bool handle(application::OnMouseUp const &) override
        {
            if (_panning)
            {
                _panning.finish(_target);
            }
            return true;
        }

        bool handle(application::OnMouseWheel const & e) override
        {
            _orbiting.updateDistance(-0.5f * e._dy);
            _orbiting.evaluate(_cameraTransform);

            auto & camNode = scene().root().at<scene::Node>({"cam-node"});
            camNode.setOrigin(_cameraTransform);

            assert(_orthographicCamera);
            float const zoom = _orbiting.distance() / 2.0f;
            _orthographicCamera->setXMag(zoom);
            _orthographicCamera->setYMag(zoom);

            return true;
        }

        bool handle(application::OnResize const & e) override
        {
            _windowSize.x = static_cast<float>(e._width);
            _windowSize.y = static_cast<float>(e._height);
            return true;
        }

        bool handle(application::OnKeyDown const & e) override
        {
            switch(e._key)
            {
                case SDLK_c:
                    _isPerspective = !_isPerspective;
                    MINIRE_INFO("Camera is switched to {}", _isPerspective ? "PERSPECTIVE" : "ORTHOGRAPHIC");
                    scene().setActiveCamera({"cam-node", _isPerspective ? "persp-cam" : "ortho-cam"});
                    break;

                case SDLK_d:
                    _isDirectLightEnabled = !_isDirectLightEnabled;
                    MINIRE_INFO("Toggle direct light: {}", _isDirectLightEnabled);
                    scene().root().at<scene::DirectionalLight>("directlight-node", "sun").setVisible(_isDirectLightEnabled);
                    break;

                case SDLK_p:
                    _isPointLightEnabled = !_isPointLightEnabled;
                    MINIRE_INFO("Toggle point light: {}", _isPointLightEnabled);
                    scene().root().at<scene::PointLight>("pointlight-node", "bulb").setVisible(_isPointLightEnabled);
                    break;

                case SDLK_f:
                    _isFloorPlaneEnabled = !_isFloorPlaneEnabled;
                    MINIRE_INFO("Toggle floor plane: {}", _isFloorPlaneEnabled);
                    scene().root().at<scene::Mesh>(models::ScenePath{"floor-node", "floor-plane"}).setVisible(_isFloorPlaneEnabled);
                    break;
            }
            return false;
        }

    private:
        glm::vec3                       _target;
        grips::Orbiting                 _orbiting;
        scene::Node::Sptr               _cameraNode;
        scene::PerspectiveCamera::Sptr  _perspectiveCamera;
        scene::OrthographicCamera::Sptr _orthographicCamera;
        models::Transform               _cameraTransform;
        grips::Panning<false>           _panning;
        glm::vec2                       _windowSize;
        bool                            _isDirectLightEnabled;
        bool                            _isPointLightEnabled;
        bool                            _isFloorPlaneEnabled;
        bool                            _isPerspective;
    };

    template<typename ApplicationType>
    int main(std::string const & title)
    {
       try
        {
            // Initialization
            logging::setVerbosity(logging::Level::kDebug);

            // Setup content manager
            content::Manager manager;
            manager.setReader<content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

            auto lease = manager.upload(kFontFace, models::FontFace
                {
                    ._regular = "../common/6x13.bdf",
                    ._bold = "../common/6x13B.bdf",
                    ._italic = "../common/6x13O.bdf",
                    ._glyphWidth = 6,
                    ._glyphHeight = 13,
                });

            // Create and run the Application and its Controller
            ApplicationType application(1280, 720, title, manager);
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
