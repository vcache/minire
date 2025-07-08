#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/fps-camera.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/scene-model.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    static size_t const kMaxCtrlFps = 1;
    static float const kVelocity = 1.0f;
    static bool const kUseTexture = true;

    class RotatingCube
        : public minire::BasicController
    {
    public:
        explicit RotatingCube(minire::events::ApplicationQueue const & applicationQueue)
            : BasicController(applicationQueue)
            , _fpsCamera(glm::vec3(10.0f, 5.0f, 10.0f),
                         glm::vec3(0.0f, 0.0f, -1.0f),
                         glm::vec2(-130.0f, 0.0f))
        {}

        // TODO: maybe use KeyCoder
        void start() override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            enqueue<SceneUpdateFpsCamera>(_fpsCamera);
            enqueue<SceneEmergeModel>(0, "cube-model", _cubePosition);
            enqueue<SceneEmergePointLight>(
                0, PointLight(glm::vec3(2.0f,  2.0f, 2.0f),
                              glm::vec4(1, 1, 1, 500), 2));
        }

        void step() override
        {
            using namespace minire::events::controller;

            float const delta = frameTime();
            _cubePosition._rotation = glm::rotate(_cubePosition._rotation,
                                                  delta * kVelocity,
                                                  glm::vec3{0, 1, 0});

            enqueue<SceneUpdateModel>(0, _cubePosition);
        }

    private:
        minire::models::FpsCamera     _fpsCamera;
        minire::models::ModelPosition _cubePosition;
    };
}

int main()
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);

        minire::content::Manager manager;
        {
            auto inMemReader = std::make_unique<minire::content::readers::InMemory>();

            using MapType = minire::models::SceneModel::Map;
            inMemReader->store("cube-model", minire::models::SceneModel
            {
                ._mesh = "cube.obj",
                ._albedo = []
                {
                    // TODO: I don't know why is this shit must be wrapper into a lambda,
                    //       but without it _albedo.index() == 255 when kUseTexture == false.
                    //       Must some bug in compiler or stl or whatever.
                    // TODO: Is "uv-color.png" a license-safe one??
                    return kUseTexture ? MapType(std::in_place_type<minire::content::Id>, "uv-color.png")
                                       : MapType(std::in_place_type<glm::vec3>, 1.0, 0.0, 0.0);
                }(),
                ._metallic = 0.5f,
                ._roughness = 0.6f,
                ._ao = 1.0f,
                ._normals = std::monostate()
            });

            manager.setReader<minire::content::readers::Chained>()
                .append(std::move(inMemReader))
                .append<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);
        }

        minire::Application application(1280, 720, "Rotating cube", manager);
        application.setController<RotatingCube>(kMaxCtrlFps);
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
