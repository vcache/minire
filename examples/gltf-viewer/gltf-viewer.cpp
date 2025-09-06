#include <minire/application.hpp>

#include <minire/basic-controller.hpp>
#include <minire/content/manager.hpp>
#include <minire/grips/orbiting.hpp>
#include <minire/grips/screen-space-panning.hpp>
#include <minire/logging.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/mesh.hpp>
#include <minire/models/pbr-material.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/transform.hpp>

#include <boost/algorithm/string.hpp> // for split
#include <boost/program_options.hpp>

#include <cstdlib> // for EXIT_SUCCESS
#include <iostream>
#include <limits>
#include <optional>

namespace
{
    const static size_t kNoIndex = std::numeric_limits<size_t>::max();

    struct Arguments
    {
        std::string _filename;
        size_t      _scene = kNoIndex;
        size_t      _node = kNoIndex;
        size_t      _mesh = kNoIndex;
        std::string _animationSceneNode;
        std::string _animationName;
        size_t      _animationRepeats = -1;
        float       _animationScale = 1.0f;
        bool        _setDefaultMaterial = false;
        bool        _showHelp = false;
    };

    namespace po = boost::program_options;

    class ArgsParser
    {
        static constexpr char const * kFilename = "filename";
        static constexpr char const * kScene = "scene";
        static constexpr char const * kNode = "node";
        static constexpr char const * kMesh = "mesh";
        static constexpr char const * kAnimationSceneNode = "animation-scene-node";
        static constexpr char const * kAnimationName = "animation-name";
        static constexpr char const * kAnimationRepeats = "animation-repeats";
        static constexpr char const * kAnimationScale = "animation-scale";
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
                (kScene,
                    po::value<size_t>()->default_value(kNoIndex),
                    "a scene index to inspect")
                (kNode,
                    po::value<size_t>()->default_value(kNoIndex),
                    "a node index to inspect")
                (kMesh,
                    po::value<size_t>()->default_value(kNoIndex),
                    "a mesh index to inspect")
                (kAnimationSceneNode,
                    po::value<std::string>()->default_value(""),
                    "a scene node where an animation to be found")
                (kAnimationName,
                    po::value<std::string>()->default_value(""),
                    "a animation to play")
                (kAnimationRepeats,
                    po::value<size_t>()->default_value(1),
                    "animation repeats (-1 for infinite loop)")
                (kAnimationScale,
                    po::value<float>()->default_value(1.0),
                    "a time scale factor for an animation")
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
            _result._scene = vm[kScene].as<size_t>();
            _result._node = vm[kNode].as<size_t>();
            _result._mesh = vm[kMesh].as<size_t>();
            _result._animationSceneNode = vm[kAnimationSceneNode].as<std::string>();
            _result._animationName = vm[kAnimationName].as<std::string>();
            _result._animationRepeats = vm[kAnimationRepeats].as<size_t>();
            _result._animationScale = vm[kAnimationScale].as<float>();
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
            , _target(0.0f, 0.0f, 0.0f)
            , _dollyGrip(_target, 10)
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

            _dollyGrip.evaluate(_cameraTransform);
            enqueue<SceneNewNode>("cam-node", ScenePath(), _cameraTransform, true);
            enqueue<SceneNewPerspectiveCamera>("cam", ScenePath{"cam-node"}, camera, true);
            enqueue<SceneActivateCamera>(ScenePath{"cam-node", "cam"});

            enqueue<SceneNewNode>("light-node", ScenePath(), Transform(glm::vec3(2.0f,  2.0f, 2.0f)), true);
            enqueue<SceneNewPointLight>("bulb", ScenePath{"light-node"}, PointLight(glm::vec4(1, 1, 1, 500), 2), true);

            enqueue<SceneNewNode>("target-node", ScenePath(), minire::models::Transform(), true);
            if (_arguments._scene != kNoIndex)
            {
                enqueue<SceneNewFromSource>(ScenePath{"target-node"},
                                            mkPath(_arguments._filename,
                                                   path::Special::kScenes,
                                                   path::Index(_arguments._scene)),
                                            true);
            }
            else if (_arguments._node != kNoIndex)
            {
                enqueue<SceneNewFromSource>(ScenePath{"target-node"},
                                            mkPath(_arguments._filename,
                                                   path::Special::kNodes,
                                                   path::Index(_arguments._node)),
                                            true);
            }
            else if (_arguments._mesh != kNoIndex)
            {
                // NOTE: in a case of meshes, there are two ways to achieve same result
#if 1
                enqueue<SceneNewFromSource>(ScenePath{"target-node"},
                                            mkPath(_arguments._filename,
                                                   path::Special::kMeshes,
                                                   path::Index(_arguments._mesh)),
                                            true);
#else
                enqueue<SceneNewMesh>("cube", ScenePath{"target-node"},
                    Mesh
                    {
                        ._source = mkPath(_arguments._filename,
                                          path::Special::kMeshes,
                                          path::Index(_arguments._mesh)),
                        ._defaultMaterial = _arguments._setDefaultMaterial
                            ? std::make_shared<minire::models::PbrMaterial>()
                            : minire::material::Model::Sptr()
                    },
                    true);
#endif
            }
            else
            {
                enqueue<SceneNewFromSource>(ScenePath{"target-node"},
                                            mkPath(_arguments._filename),
                                            true);
            }

            if (!_arguments._animationName.empty())
            {
                minire::models::ScenePath containerNode;
                boost::split(containerNode, _arguments._animationSceneNode, boost::is_any_of("/"));
                enqueue<ScenePlayAnimation>(_arguments._animationName,
                                            containerNode,
                                            _arguments._animationRepeats,
                                            _arguments._animationScale);
            }
        }

        void handle(minire::events::application::OnMouseMove const & event) override
        {
            using namespace minire::events::controller;
            using namespace minire::models;

            bool updated = false;
            if (event._left)
            {
                _dollyGrip.updateAngles(0.01f * static_cast<float>(event._relX),
                                        0.01f * static_cast<float>(event._relY));
                updated = true;
            }
            else if (event._right && _panning)
            {
                _dollyGrip.target() = _panning.update(event._absX, event._absY,
                                                      _cameraTransform.matrix(),
                                                      _target);
                updated = true;
            }

            if (updated)
            {
                _dollyGrip.evaluate(_cameraTransform);
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

            _dollyGrip.updateDistance(-0.5f * event._dy);
            _dollyGrip.evaluate(_cameraTransform);
            enqueue<SceneSetTransform>(ScenePath{"cam-node"}, _cameraTransform);
        }

    private:
        Arguments const &                 _arguments;
        minire::models::Transform         _cameraTransform;
        glm::vec3                         _target;
        minire::grips::Orbiting           _dollyGrip;
        minire::grips::ScreenSpacePanning _panning;
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

        size_t const groupsCount =
            (arguments._scene != kNoIndex ? 1 : 0) +
            (arguments._node != kNoIndex ? 1 : 0) +
            (arguments._mesh != kNoIndex ? 1 : 0);
        MINIRE_INVARIANT(groupsCount <= 1,
                         "choose either --scene, --node or --mesh");

        MINIRE_INVARIANT(!arguments._filename.empty(),
                         "a file to be opened isn't specified");

        // Setup content manager
        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);

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
