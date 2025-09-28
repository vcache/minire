#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/mesh.hpp>
#include <minire/models/pbr-material.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/transform.hpp>

#include <boost/program_options.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <cstdlib> // for EXIT_SUCCESS
#include <iostream>

namespace
{
    struct Arguments
    {
        size_t _maxCtrlFps;
        float  _velocity;
        bool   _useTexture;
        bool   _useGltf;
        bool   _showHelp;
    };

    namespace po = boost::program_options;

    class ArgsParser
    {
        static constexpr char const * kMaxCtrlFps = "max-ctrl-fps";
        static constexpr char const * kVelocity = "velocity";
        static constexpr char const * kUseTexture = "use-texture";
        static constexpr char const * kUseGltf = "use-gltf";
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
                (kUseGltf,
                    po::value<bool>()->default_value(false),
                    "should read a mesh from the GLTF file")
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
            _result._useGltf = vm[kUseGltf].as<bool>();
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
        explicit RotatingCube(minire::content::Manager & contentManager,
                              Arguments const & arguments)
            : BasicController(contentManager, arguments._maxCtrlFps)
            , _arguments(arguments)
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
                    ._source = _arguments._useGltf ? mkPath("cube.gltf", path::Special::kMeshes, path::Index(0))
                                                   : mkPath("cube.obj"),
                    ._defaultMaterial = [this]
                    {
                        auto result = std::make_shared<PbrMaterial>();
                        if (_arguments._useTexture)
                        {
                            result->_albedoTexture =  "uv-color.png";
                        }
                        else
                        {
                            result->_albedoFactor = glm::vec3(1.0f, 0.0f, 0.0f);
                        }
                        result->_metallicFactor = 0.5f;
                        result->_roughnessFactor = 0.6f;
                        return result;
                    }()
                },
                true);
        }

        void step() override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            float const delta = frameTime();
            _cubeTransform._rotation = glm::rotate(_cubeTransform._rotation,
                                                   delta * _arguments._velocity,
                                                   glm::vec3{0, 1, 0});
            enqueue<SceneSetTransform>(ScenePath{"cube-node"}, _cubeTransform);
        }

    private:
        Arguments const &         _arguments;
        minire::models::Transform _cubeTransform;
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
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

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
