#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/grips/orbiting.hpp>
#include <minire/grips/panning.hpp>
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
        : public minire::Application
    {
    public:
        template<typename... AppArgs>
        explicit GltfViewer(Arguments const & arguments,
                            AppArgs && ... appArgs)
            : Application(std::forward<AppArgs>(appArgs)...)
            , _arguments(arguments)
            , _target(0.0f, 0.0f, 0.0f)
            , _orbiting(_target, 10)
        {}

        void onStart() override
        {
            using namespace minire::content;
            using namespace minire::models;

            Application::onStart();

            Transform initialCameraTransform;
            _orbiting.evaluate(initialCameraTransform);

            _cameraNode = scene().root().make(Node{initialCameraTransform, true});
            _camera = _cameraNode->make(
                minire::models::PerspectiveCamera
                {
                    ._yFov = glm::radians(45.0f),
                    ._zNear = 0.001f,
                    ._zFar = 1000.0f,
                    ._aspectRatio = std::nullopt,
                    ._visible = true,
                });
            _camera->activate();

            auto lightNode = scene().root().make(Node{Transform(glm::vec3(2.0f,  2.0f, 2.0f)), true});
            lightNode->make(PointLight(glm::vec4(1, 1, 1, 500), 2, std::nullopt, true));

            auto targetNode = scene().root().make("target", Node{minire::models::Transform(), true});
            if (_arguments._scene != kNoIndex)
            {
                targetNode->makeFromSource(mkPath(_arguments._filename,
                                                  path::Special::kScenes,
                                                  path::Index(_arguments._scene)),
                                           contentManager(), true);
            }
            else if (_arguments._node != kNoIndex)
            {
                targetNode->makeFromSource(mkPath(_arguments._filename,
                                                  path::Special::kNodes,
                                                  path::Index(_arguments._node)),
                                           contentManager(), true);
            }
            else if (_arguments._mesh != kNoIndex)
            {
                // NOTE: in a case of meshes, there are two ways to achieve same result
#if 1
                targetNode->makeFromSource(mkPath(_arguments._filename,
                                                  path::Special::kMeshes,
                                                  path::Index(_arguments._mesh)),
                                           contentManager(), true);
#else
                targetNode->makeFromSource(
                    Mesh
                    {
                        ._source = mkPath(_arguments._filename,
                                          path::Special::kMeshes,
                                          path::Index(_arguments._mesh)),
                        ._defaultMaterial = _arguments._setDefaultMaterial
                            ? std::make_shared<minire::models::PbrMaterial>()
                            : minire::material::Model::Sptr()
                        ._skin = std::nullopt,
                        ._visible = true,
                    },
                    true);
#endif
            }
            else
            {
                targetNode->makeFromSource(mkPath(_arguments._filename), contentManager(), true);
            }

            if (!_arguments._animationName.empty())
            {
                minire::models::ScenePath containerNode;
                boost::split(containerNode, _arguments._animationSceneNode, boost::is_any_of("/"));
                scene().root().at<minire::scene::Node>(containerNode)
                              .playAnimation(_arguments._animationName,
                                             _arguments._animationRepeats,
                                             _arguments._animationScale);
            }
        }

        bool handle(minire::application::OnMouseMove const & e) override
        {
            using namespace minire::models;

            if (Application::handle(e))
                return true;

            bool updated = false;
            if (e._left)
            {
                _orbiting.updateAngles(0.01f * static_cast<float>(e._relX),
                                       0.01f * static_cast<float>(e._relY));
                updated = true;
            }
            else if (e._right && _panning)
            {
                _orbiting.target() = _panning.update(e._absX, e._absY,
                                                     _cameraNode->origin().matrix(),
                                                     _target, _camera->model(), _windowSize,
                                                     _cameraNode->origin()._translation);
                updated = true;
            }

            if (updated)
            {
                _orbiting.evaluate(_cameraNode->origin());
            }

            return true;
        }

        bool handle(minire::application::OnMouseDown const & e) override
        {
            if (Application::handle(e))
                return true;

            if (e._mouseButton == minire::models::MouseButton::kRight)
            {
                _panning.start(e._x, e._y);
            }

            return true;
        }

        bool handle(minire::application::OnMouseUp const & e) override
        {
            if (Application::handle(e))
                return true;

            if (_panning)
            {
                _panning.finish(_target);
            }

            return true;
        }

        bool handle(minire::application::OnMouseWheel const & e) override
        {
            using namespace minire::models;

            if (Application::handle(e))
                return true;

            _orbiting.updateDistance(-0.5f * e._dy);
            _orbiting.evaluate(_cameraNode->origin());

            return true;
        }

        bool handle(minire::application::OnResize const & e) override
        {
            if (Application::handle(e))
                return true;

            _windowSize.x = static_cast<float>(e._width);
            _windowSize.y = static_cast<float>(e._height);

            return true;
        }

    private:
        Arguments const &                      _arguments;
        minire::scene::Node::Sptr              _cameraNode;
        minire::scene::PerspectiveCamera::Sptr _camera;
        glm::vec3                              _target;
        minire::grips::Orbiting                _orbiting;
        minire::grips::Panning<false>          _panning;
        glm::vec2                              _windowSize;
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
        GltfViewer application(arguments, 1280, 720, "gLTF Viewer", manager);
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
