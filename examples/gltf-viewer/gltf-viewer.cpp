#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/fps-camera.hpp>
#include <minire/models/pbr-material.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/scene-model.hpp>

#include <boost/program_options.hpp>

#include <cstdlib> // for EXIT_SUCCESS
#include <iostream>

namespace
{
    struct Arguments
    {
        std::string _filename;
        size_t      _mesh = 0;
        bool        _setDefaultMaterial = false;
        bool        _showHelp = false;
    };

    namespace po = boost::program_options;

    class ArgsParser
    {
        static constexpr char const * kFilename = "filename";
        static constexpr char const * kMesh = "mesh";
        static constexpr char const * kSetDefaultMaterial = "set-default-material";
        static constexpr char const * kHelp = "help";

    public:
        ArgsParser(int argc, char * argv[])
            : _desc("A gLTF viewing utility.\n"
                    "Usage:\n  ./gltf-viewer [options] <filename>\n\n"
                    "For example:\n  ./gltf-viewer --mesh=0 ./assets/Cube/Cube.gltf\n\n"
                    "Options")
        {
            _desc.add_options()
                (kFilename,
                    po::value<std::string>(),
                    "a filename to open")
                (kMesh,
                    po::value<size_t>()->default_value(0),
                    "a mesh index to inspect")
                (kSetDefaultMaterial,
                    po::value<bool>()->default_value(false),
                    "set default material for material-less meshes")
                (kHelp,
                    "print this message");

            po::positional_options_description pod;
            pod.add(kFilename, 1);

            po::variables_map vm;
            po::store(po::command_line_parser(argc, argv)
                        .options(_desc)
                        .positional(pod)
                        .run(),
                      vm);
            po::notify(vm);

            _result._filename = vm.count(kFilename) ? vm[kFilename].as<std::string>() : "";
            _result._mesh = vm[kMesh].as<size_t>();
            _result._setDefaultMaterial = vm[kSetDefaultMaterial].as<bool>();
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
    class GltfViewer
        : public minire::BasicController
    {
    public:
        explicit GltfViewer(Arguments const & arguments)
            : BasicController(60)
            , _arguments(arguments)
            , _fpsCamera(glm::vec3(5.0f, 2.5f, 5.0f),
                         glm::vec3(0.0f, 0.0f, -1.0f),
                         glm::vec2(-130.0f, 0.0f))
        {}

        void start() override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            enqueue<SceneUpdateFpsCamera>(_fpsCamera);
            enqueue<SceneEmergePointLight>(
                0, PointLight(glm::vec3(2.0f,  2.0f, 2.0f),
                              glm::vec4(1, 1, 1, 500), 2));
            enqueue<SceneEmergeModel>(0, "test-sample", _origin);
        }

        void handle(minire::events::application::OnMouseMove const & event)
        {
            using namespace minire::events::controller;

            // TODO: rotations are mess, fix it!
            // TODO: add zoom in/out
            // TODO: add panning
            // TODO: scale factor must depend on a zoom factor (or camera scale)

            if (event._left)
            {
                _origin._rotation = glm::rotate(_origin._rotation,
                                                0.005f * static_cast<float>(event._relX),
                                                glm::vec3{0, 1, 0});

                _origin._rotation = glm::rotate(_origin._rotation,
                                                0.005f * static_cast<float>(event._relY),
                                                glm::vec3(std::sqrt(2.0) / 2.0, 0, -std::sqrt(2.0) / 2.0));

                enqueue<SceneUpdateModel>(0, _origin);
            }
        }

    private:
        Arguments const &             _arguments;
        minire::models::FpsCamera     _fpsCamera;
        minire::models::ModelPosition _origin;
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

        MINIRE_INVARIANT(!arguments._filename.empty(),
                         "a file to be opened isn't specified");

        // Setup content manager
        auto inMemReader = std::make_unique<minire::content::readers::InMemory>();
        inMemReader->store(
            "test-sample",
            minire::models::SceneModel
            {
                ._source = arguments._filename,
                ._meshIndex = arguments._mesh,
                ._defaultMaterial = arguments._setDefaultMaterial
                    ? std::make_shared<minire::models::PbrMaterial>()
                    : minire::material::Model::Sptr()
            });

        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Chained>()
               .append(std::move(inMemReader))
               .append<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

        // Create and run the Application and its Controller
        minire::Application application(1280, 720, "gLTF Viewer", manager);
        application.setController<GltfViewer>(arguments);
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
