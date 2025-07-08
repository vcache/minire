#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/fps-camera.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/scene-model.hpp>

#include <boost/program_options.hpp>

#include <cstdlib> // for EXIT_SUCCESS
#include <iostream>

namespace
{
    struct Arguments
    {
        size_t _maxCtrlFps;
        float  _velocity;
        bool   _useTexture;
        bool   _showHelp;
    };

    namespace po = boost::program_options;

    class ArgsParser
    {
        static constexpr char const * kMaxCtrlFps = "max-ctrl-fps";
        static constexpr char const * kVelocity = "velocity";
        static constexpr char const * kUseTexture = "use-texture";
        static constexpr char const * kHelp = "help";

    public:
        ArgsParser(int argc, char * argv[])
            : _desc("The rotating cube example.\n"
                    "Usage: ./rotating-cube [options]\n"
                    "\nOptions")
        {
            _desc.add_options()
                (kMaxCtrlFps,
                    po::value<size_t>()->default_value(10),
                    "FPS of a controller (main loop frequency)")
                (kVelocity,
                    po::value<float>()->default_value(1.0f),
                    "a rotation velocity")
                (kUseTexture,
                    po::value<bool>()->default_value(false),
                    "should a box be painted by a texture")
                (kHelp,
                    "print this message");

            po::variables_map vm;
            po::store(po::command_line_parser(argc, argv)
                            .options(_desc)
                            .run(),
                      vm);
            po::notify(vm);

            _result._maxCtrlFps = vm[kMaxCtrlFps].as<size_t>();
            _result._velocity = vm[kVelocity].as<float>();
            _result._useTexture = vm[kUseTexture].as<bool>();
            _result._showHelp = vm.count(kHelp) != 0;
        }

        void printHelp() const
        {
            std::cout << _desc << std::endl;
        }

        Arguments const & arguments() const { return _result; }

    private:
        po::options_description _desc;
        Arguments               _result;
    };
}

namespace
{
    class RotatingCube
        : public minire::BasicController
    {
    public:
        explicit RotatingCube(Arguments const & arguments)
            : BasicController(arguments._maxCtrlFps)
            , _arguments(arguments)
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
                                                  delta * _arguments._velocity,
                                                  glm::vec3{0, 1, 0});

            enqueue<SceneUpdateModel>(0, _cubePosition);
        }

    private:
        Arguments const &             _arguments;
        minire::models::FpsCamera     _fpsCamera;
        minire::models::ModelPosition _cubePosition;
    };
}

int main(int argc, char ** argv)
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);

        // Parse CLI arguments
        ArgsParser argsParser(argc, argv);
        Arguments const arguments = argsParser.arguments();
        if (arguments._showHelp)
        {
            argsParser.printHelp();
            return EXIT_SUCCESS;
        }

        // Setup content manager
        minire::content::Manager manager;
        {
            auto inMemReader = std::make_unique<minire::content::readers::InMemory>();

            using MapType = minire::models::SceneModel::Map;
            inMemReader->store("cube-model", minire::models::SceneModel
            {
                ._mesh = "cube.obj",
                ._albedo = [&arguments]
                {
                    // TODO: I don't know why is this shit must be wrapper into a lambda,
                    //       but without it _albedo.index() == 255 when kUseTexture == false.
                    //       Must some bug in compiler or stl or whatever.
                    // TODO: Is "uv-color.png" a license-safe one??
                    return arguments._useTexture
                        ? MapType(std::in_place_type<minire::content::Id>, "uv-color.png")
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

        // Create and run the Application and its Controller
        minire::Application application(1280, 720, "Rotating cube", manager);
        application.setController<RotatingCube>(arguments);
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
