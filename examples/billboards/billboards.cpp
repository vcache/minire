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

#include <fmt/format.h>

#include <cstdlib> // for EXIT_SUCCESS
#include <vector>

namespace
{
    static size_t constexpr kCtrlFps = 30;
    static std::string const kFontFace = "ucs-6x13-example";

    // TODO: This code is duplicated in other examples,
    //       consider to make a base controller w/ orbiting, panning and dual-camera capabilities.

    class Billboards
        : public minire::BasicController
    {
    public:
        explicit Billboards(minire::content::Manager & contentManager)
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

        auto makeText() const
        {
            minire::text::FormattedString text;
            // TODO: alpha channel
            // TODO: different styles
            text.append(L"Hello world").background(glm::vec4(0, 0, 0, 0))
                                       .foreground(glm::vec4(1, 0, 0, 0));
            return text;
        }

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

            size_t counter = 0;
            auto mkSample = [&](glm::vec3 origin,
                                glm::vec3 billboardNodeOrigin,
                                std::vector<Billboard> billboards)
            {
                std::string rootNode = fmt::format("cube-node-{}", counter++);
                enqueue<SceneNewNode>(rootNode, ScenePath(), Transform(origin), true);
                enqueue<SceneNewMesh>("cube", ScenePath{rootNode},
                    Mesh
                    {
                        ._source = mkPath("Box.glb", path::Special::kMeshes, path::Index(0)),
                        ._defaultMaterial = nullptr,
                    },
                    true);

                enqueue<SceneNewNode>("billboard-node", ScenePath{rootNode}, Transform(billboardNodeOrigin), true);
                for(Billboard const & billboard : billboards)
                {
                    enqueue<SceneNewBillboard>(fmt::format("billboard-{}", (void const *)&billboard),
                                               ScenePath{rootNode, "billboard-node"},
                                               billboard, true);
                }
            };

            static minire::utils::NinePatch const kNinePatch
            {
                ._boundary = minire::utils::Rect(55, 49, 158, 152),
                ._out = minire::utils::Rect(69, 63, 144, 138),
                ._in = minire::utils::Rect(72, 66, 141, 135),
            };

            static minire::utils::Rect const kRect(8, 6, 39, 37);

            // World placement //

            {
                // Whole image

                mkSample(glm::vec3(-4, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"single-box.png", std::monostate(), {}},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                mkSample(glm::vec3(-2, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"single-rectangular.png", std::monostate(), {}},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                // Part of image

                mkSample(glm::vec3(0, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kRect, {}},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                // NinePatch
                mkSample(glm::vec3(2, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kNinePatch, glm::vec2(200, 200)},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                // Z-Ordering
                mkSample(glm::vec3(4, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kNinePatch, glm::vec2(200, 200)},
                                    Billboard::World({0, .25}, {.5, .5}), 0},
                          Billboard{Billboard::Sprite{"single-box.png", std::monostate(), {}},
                                    Billboard::World({0, .25}, {.4, .4}), 1},
                          Billboard{Billboard::Sprite{"atlas.png", kRect, {}},
                                    Billboard::World({0, .25}, {.3, .3}), 2}});

                // Text

                mkSample(glm::vec3(6, 0, 0), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Label{makeText(), kFontFace},
                                    Billboard::World({0, .25}, {.5, .5}), 0}});

                // TODO: Animations (scale, rotate, translate, update image)
            }

            // Screen placement //

            {
                // Whole image

                mkSample(glm::vec3(-4, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"single-box.png", std::monostate(), {}},
                                    Billboard::Screen({0, 16}), 0}});

                mkSample(glm::vec3(-2, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"single-rectangular.png", std::monostate(), {}},
                                    Billboard::Screen({0, 16}), 0}});

                // Part of image

                mkSample(glm::vec3(0, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kRect, {}},
                                    Billboard::Screen({0, 16}), 0}});

                // NinePatch

                mkSample(glm::vec3(2, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kNinePatch, glm::vec2(200, 200)},
                                    Billboard::Screen({0, 100}), 0}});

                // Z-Ordering

                mkSample(glm::vec3(4, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Sprite{"atlas.png", kNinePatch, glm::vec2(200, 200)},
                                    Billboard::Screen({0, 100}), 0},
                          Billboard{Billboard::Sprite{"single-box.png", std::monostate(), {}},
                                    Billboard::Screen({0, 50}), 1},
                          Billboard{Billboard::Sprite{"atlas.png", kRect, {}},
                                    Billboard::Screen({10, 60}), 2}});

                // Text

                mkSample(glm::vec3(6, 0, 2), glm::vec3(0, .5, 0),
                         {Billboard{Billboard::Label{makeText(), kFontFace},
                                    Billboard::Screen({0, 10}), 0}});

                // TODO: Animations (scale, rotate, translate, update image)
            }

            // TODO: Screen + World mixed placement
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
            if (e._key == SDLK_c)
            {
                _isPerspective = !_isPerspective;
                MINIRE_INFO("Camera is switched to {}", _isPerspective ? "PERSPECTIVE"
                                                                       : "ORTHOGRAPHIC");
                using namespace minire::events::controller;
                using namespace minire::models;
                enqueue<SceneActivateCamera>(ScenePath{"cam-node", _isPerspective ? "persp-cam" : "ortho-cam"});
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

        auto lease = manager.upload(kFontFace, minire::models::FontFace
            {
                ._regular = "../common/6x13.bdf",
                ._bold = "../common/6x13B.bdf",
                ._italic = "../common/6x13O.bdf",
                ._glyphWidth = 6,
                ._glyphHeight = 13,
            });

        // Create and run the Application and its Controller
        minire::Application application(1280, 720, "Camera wwitch", manager);
        application.setController<Billboards>();
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
