#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/mesh.hpp>
#include <minire/models/pbr-material.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/transform.hpp>

#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <cstdlib> // for EXIT_SUCCESS
#include <iostream>

namespace
{
    class AmbientLight
        : public minire::BasicController
    {
    private:
        minire::models::Transform cameraTransform(float dist = 5.0f)
        {
            minire::models::Transform result(glm::vec3(dist, dist, dist));
            result._rotation = glm::rotate(result._rotation,
                                           glm::radians(45.0f),
                                           glm::vec3(0, 1, 0));
            // see Rectangular cuboid side (an angle between side's diagonal and main diagonal)
            float const innerAngle =
                std::asin(std::sqrt(2.0*dist) / std::sqrt(3.0*dist)) * 180.0 / M_PI;
            result._rotation = glm::rotate(result._rotation,
                                           -glm::radians(180.0f - (90.0f + innerAngle)),
                                           glm::vec3(1, 0, 0));
            return result;
        }

    public:
        explicit AmbientLight(minire::content::Manager & contentManager)
            : BasicController(contentManager, 60)
            , _sceneAmbientLights
            {
                glm::vec3(0, 0, 0),
                glm::vec3(0, 0, 1),
                glm::vec3(0, 1, 0),
                glm::vec3(0, 1, 1),
                glm::vec3(1, 0, 0),
                glm::vec3(1, 0, 1),
                glm::vec3(1, 1, 0),
                glm::vec3(1, 1, 1),
            }
        {}

        void start() override
        {
            using namespace minire::content;
            using namespace minire::events::controller;
            using namespace minire::models;

            minire::models::PerspectiveCamera camera{._yFov = glm::radians(45.0f),
                                                     ._zNear = 0.001f,
                                                     ._zFar = 1000.0f,
                                                     ._aspectRatio = std::nullopt};

            enqueue<SceneNewNode>("cam-node", ScenePath(), cameraTransform(5.0f), true);
            enqueue<SceneNewPerspectiveCamera>("cam", ScenePath{"cam-node"}, camera, true);
            enqueue<SceneActivateCamera>(ScenePath{"cam-node", "cam"});

            enqueue<SceneNewNode>("light-node", ScenePath(), Transform(glm::vec3(2.0f,  2.0f, 2.0f)), true);
            enqueue<SceneNewPointLight>("bulb", ScenePath{"light-node"}, PointLight(glm::vec4(1, 1, 1, 500), 2), true);

            enqueue<SceneNewNode>("cube-node", ScenePath(), _cubeTransform, true);
            enqueue<SceneNewMesh>("cube", ScenePath{"cube-node"},
                Mesh
                {
                    ._source = mkPath("cube.gltf", path::Special::kMeshes, path::Index(0)),
                    ._defaultMaterial = [this]
                    {
                        auto result = std::make_shared<PbrMaterial>();
                        result->_albedoTexture =  "uv-color.png";
                        result->_metallicFactor = 0.5f;
                        result->_roughnessFactor = 0.6f;
                        return result;
                    }()
                },
                true);

            enqueue<SceneSetAmbientLight>(_sceneAmbientLights[0]);
        }

        bool handle(minire::events::application::OnKeyDown const & e) override
        {
            using namespace minire::events::controller;

            if (BasicController::handle(e))
                return true;

            if (e._key == SDLK_TAB)
            {
                _sceneAmbientLight = (_sceneAmbientLight + 1) % _sceneAmbientLights.size();
                MINIRE_INFO("Scene ambient light is set to ({}): {}",
                            _sceneAmbientLight, _sceneAmbientLights[_sceneAmbientLight]);
                enqueue<SceneSetAmbientLight>(_sceneAmbientLights[_sceneAmbientLight]);
            }

            return true;
        }

        void step() override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            float const delta = frameTime();
            _absoluteTime += delta;
            _cubeTransform._rotation = glm::rotate(_cubeTransform._rotation,
                                                   delta * 0.5f,
                                                   glm::vec3{0, 1, 0});
            enqueue<SceneSetTransform>(ScenePath{"cube-node"}, _cubeTransform);

            float const w = (1.0f + std::sin(_absoluteTime * 10.0f)) / 2.0f;
            enqueue<SceneSetMeshAmbientLight>(ScenePath{"cube-node", "cube"},
                                              glm::vec4(0, 1, 0, w));
        }

    private:
        size_t                       _sceneAmbientLight = 0;
        std::vector<glm::vec3> const _sceneAmbientLights;
        minire::models::Transform    _cubeTransform;
        float                        _absoluteTime = 0;
    };
}

int main()
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);

        // Setup content manager
        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

        // Create and run the Application and its Controller
        minire::Application application(1280, 720, "Ambient Light", manager);
        application.setController<AmbientLight>();
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
