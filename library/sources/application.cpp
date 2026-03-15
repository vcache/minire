#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/instrumentation/formatters.hpp>
#include <minire/instrumentation/stopwatch.hpp>
#include <minire/logging.hpp>
#include <minire/utils/geometry.hpp>
#include <minire/utils/ray-caster.hpp>
#include <minire/utils/unow.hpp>

#include <opengl.hpp>
#include <rasterizer.hpp>
#include <scene.hpp>
#include <scene/gltf-instantiator.hpp>
#include <scene/viewpoint.hpp>
#include <text/measurer.hpp>
#include <utils/overloaded.hpp>

#include <fmt/format.h>

#include <algorithm>

namespace minire
{
    // TODO: read them from Camera
    static const float kNear = 0.0f;
    static const float kFar = 1.0f;

#   ifdef NDEBUG
    constexpr static bool kDebug = false;
#   else
    constexpr static bool kDebug = true;
#   endif

    Application::Application(int width, int height,
                             std::string const & title,
                             content::Manager & contentManager,
                             models::MsaaParams const & msaaParams)
        : sdl::GlApplication(width, height, title, msaaParams)
        , _contentManager(contentManager)
        , _rasterizer(std::make_unique<Rasterizer>(contentManager))
        , _scene(std::make_unique<SceneImpl>(*_rasterizer))
    {
        setVsync(true); // TODO: into parameters

        MINIRE_GL(glClearColor, 0.0f, 0.2f, 0.2f, 1.0f); // TODO: into parameters
        MINIRE_GL(glDepthRangef, kNear, kFar);

        onResize(width, height);
        setGlDebug(true);   // TODO: disable for release
                            // TODO: into parameters

        {
            GLint param;
            MINIRE_GL(glGetIntegerv, GL_ACTIVE_TEXTURE, &param);
            MINIRE_INFO("GL_ACTIVE_TEXTURE = {}", param);

            MINIRE_GL(glGetIntegerv, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &param);
            MINIRE_INFO("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS = {}", param);
        }

        ::SDL_StopTextInput();

        _frameBegin = utils::uNow();
        startEpoch(); // just an initialization (actual start will be in onStart)

        MINIRE_INFO("Minire Application started, [{}] build", kDebug ? "DEBUG" : "RELEASE");

        if (kDebug)
        {
            enableInstrumentation();
        }

        if (!kDebug)
        {
            opengl::setGlErrorCheckMode(false);
        }
    }

    Application::~Application() = default;

    void Application::maybeIssueRayCaster()
    {
        assert(_scene);
        if (_rayCasterEnabled && _rayCasterLastEpoch < _epochNumber)
        {
            scene::Viewpoint const & vp = _scene->viewpoint();
            if (size_t const vpRevision = vp.revision();
                _rayCasterRevision < vpRevision)
            {
                _rayCaster = std::make_shared<utils::RayCaster>(
                    vp.width(), vp.height(), vp.view(), vp.projection());
                _rayCasterRevision = vpRevision;
                _rayCasterLastEpoch = _epochNumber;
            }
        }
    }

    void Application::enableInstrumentation()
    {
        if (!_timekeeper)
        {
            _timekeeper = std::make_shared<instrumentation::Histogram<>>(
                std::chrono::seconds(5));
        }
    }

    void Application::disableInstrumentation()
    {
        _timekeeper.reset();
    }

    void Application::debugDrawsUpdate(std::vector<float> const & linesBuffer)
    {
        assert(_rasterizer);
        _rasterizer->lines().update(linesBuffer);
    }


    void Application::newResourceLayer(std::string const & name)
    {
        assert(_rasterizer);
        _rasterizer->newResourceLayer(name);
        _contentManager.newLayer(name);
    }

    void Application::disposeResourceLayer(std::string const & name)
    {
        assert(_rasterizer);
        _rasterizer->disposeResourceLayer(name);
        _contentManager.disposeLayer(name);
    }

    void Application::contentManagerCleanup(bool const force)
    {
        _contentManager.cleanup(force);
    }

    // TODO: RAII-approach for vertexBuffer
    // TODO: rename to makeVertexBuffer
    void Application::createVertexBuffer(content::Id const & id,
                                         models::VertexBuffer vertexBuffer,
                                         bool const override)
    {
        assert(_rasterizer);
        _rasterizer->vertexBuffers().create(id, vertexBuffer, override); // TODO: rename to make()
    }

    void Application::disposeVertexBuffer(content::Id const & id)
    {
        _rasterizer->vertexBuffers().dispose(id);
    }

    Sprite::Sptr Application::makeSprite(std::string const & name,
                                         models::Sprite model)
    {
        assert(_rasterizer);
        return _rasterizer->sprites().make(name, std::move(model));
    }

    Sprite::Sptr const & Application::findSprite(std::string const & name)
    {
        assert(_rasterizer);
        return _rasterizer->sprites().find(name);
    }

    Sprite::Sptr const & Application::detachSprite(std::string const & name)
    {
        assert(_rasterizer);
        Sprite::Sptr const & result = _rasterizer->sprites().find(name);
        if (result)
        {
            result->detach();
        }
        return result;
    }

    Label::Sptr Application::makeLabel(std::string const & name,
                                       models::Label model)
    {
        assert(_rasterizer);
        return _rasterizer->labels().make(name, std::move(model));
    }

    Label::Sptr const & Application::findLabel(std::string const & name)
    {
        assert(_rasterizer);
        return _rasterizer->labels().find(name);
    }
    
    Label::Sptr const & Application::detachLabel(std::string const & name)
    {
        assert(_rasterizer);
        Label::Sptr const & result = _rasterizer->labels().find(name);
        if (result)
        {
            result->detach();
        }
        return result;
    }

    glm::vec2 Application::measure(text::FormattedString const & text,
                                   content::Id const & fontFace) const
    {
        auto lease = _contentManager.borrow(fontFace);
        assert(lease);
        models::FontFace const & fontData = lease->as<models::FontFace>();
        return text::measure(text, fontData);
    }

    Scene & Application::scene() const
    {
        assert(_scene);
        return *_scene;
    }

    void Application::setRayCaster(bool enabled)
    {
        _rayCasterEnabled = enabled;
        if (_rayCasterEnabled)
        {
            _rayCasterRevision = 0;
        }
    }

    void Application::onResize(size_t width, size_t height)
    {
        GlApplication::onResize(width, height);

        // required for a Projection matrix
        _scene->setViewport(width, height);

        // projection for 2D gui
        _rasterizer->setScreenSize(width, height);

        handle(application::OnResize{width, height});
    }

    void Application::onMouseWheel(int dx, int dy, uint32_t dir, ::SDL_Keymod mod)
    {
        GlApplication::onMouseWheel(dx, dy, dir, mod);
        handle(application::OnMouseWheel{._dx = dx, ._dy = dy, ._dir = dir, ._mod = mod});
    }

    void Application::onMouseMove(int absX, int absY, int relX, int relY,
                                  bool left, bool middle, bool right,
                                  bool x1, bool x2)
    {
        GlApplication::onMouseMove(absX, absY, relX, relY, left, middle, right, x1, x2);
        handle(application::OnMouseMove
        {
            ._absX = absX,
            ._absY = absY,
            ._relX = relX,
            ._relY = relY,
            ._left = left,
            ._middle = middle,
            ._right = right,
            ._x1 = x1,
            ._x2 = x2,
        });
    }

    void Application::onMouseDown(int x, int y, bool doubleClick,
                                  models::MouseButton mouseButton)
    {
        GlApplication::onMouseDown(x, y, doubleClick, mouseButton);
        handle(application::OnMouseDown{._x = x, ._y = y,
                                        ._mouseButton = mouseButton,
                                        ._doubleClick = doubleClick});
    }

    void Application::onMouseUp(int x, int y, bool doubleClick,
                               models::MouseButton mouseButton)
    {
        GlApplication::onMouseUp(x, y, doubleClick, mouseButton);
        handle(application::OnMouseUp{._x = x, ._y = y,
                                      ._mouseButton = mouseButton,
                                      ._doubleClick = doubleClick});
    }

    void Application::onKeyUp(::SDL_Keycode key, ::SDL_Scancode code, uint16_t mod)
    {
        GlApplication::onKeyUp(key, code, mod);
        handle(application::OnKeyUp{._key = key, ._code = code, ._mod = mod});
    }

    void Application::onKeyDown(::SDL_Keycode key, ::SDL_Scancode code, uint16_t mod)
    {
        GlApplication::onKeyDown(key, code, mod);
        handle(application::OnKeyDown{._key = key, ._code = code, ._mod = mod});
    }

    void Application::onTextInput(std::string const & text)
    {
        GlApplication::onTextInput(text);
        if (!text.empty())
        {
            handle(application::OnTextInput{._text = text});
        }
    }

    void Application::startEpoch()
    {
        size_t const now = utils::uNow();
        assert(now >= _epochBegin);
        _epochDuration = static_cast<double>(now - _epochBegin) / 1000000.0; // sec;
        _epochBegin = now;
        _epochNumber++;
        _epochPlayed = 0;
        _epochStarted = true;
    }

    void Application::onStart()
    {
        // starting an actual epoch (just before onRender)
        startEpoch();
        _epochDuration = 0;     // initial epoch has no duration,
    }

    void Application::onRender()
    {
        assert(_scene);

        auto totalStopwatch =
            _timekeeper ? std::make_unique<instrumentation::Stopwatch<>>("total", _timekeeper)
                        : std::unique_ptr<instrumentation::Stopwatch<>>();

        // maybe issue a ray caster
        {
            instrumentation::Stopwatch<> stopwatch("ray-caster", _timekeeper);
            maybeIssueRayCaster();
        }

        // perform controller's logic
        {
            instrumentation::Stopwatch<> stopwatch("step", _timekeeper);
            if (onStep())
            {
                startEpoch();
            }
        }

        // transfer accumulated models state into scene instances
        if (_epochStarted)
        {
            instrumentation::Stopwatch<> stopwatch("models-revalidation", _timekeeper);
            _scene->revalidateModels(_epochNumber);
        }

        // fetch and handle events from controller if any
        bool performLerp = _epochPlayed < _epochDuration;
        if (_epochStarted)
        {
            instrumentation::Stopwatch<> stopwatch("animation-advance", _timekeeper);
            performLerp |= _scene->advanceAnimations(_animationGap, _epochNumber);
            _animationGap = 0;
            _epochStarted = false;
        }

        if (performLerp)
        {
            instrumentation::Stopwatch<> stopwatch("scene-lerping", _timekeeper);
            double const weight = _epochDuration != 0 ? _epochPlayed / _epochDuration : 1.0;
            assert(weight >= 0);
            _scene->lerp(weight, _epochNumber);
        }

        {
            instrumentation::Stopwatch<> stopwatch("scene-revalidation", _timekeeper);
            _scene->revalidateNodes();
        }

        // draw a frame
        {
            instrumentation::Stopwatch<> stopwatch("scene-rendering", _timekeeper);
            _rasterizer->draw(*_scene);
            ::SDL_GL_SwapWindow(window());
        }

        // calc frame time
        size_t const frameEnd = utils::uNow();
        _frameTime = double(frameEnd - _frameBegin) / 1000000.0; // sec
        _frameTime = std::min(1.0, _frameTime); // prevent from going haywire
        _frameBegin = frameEnd;

        // advance interpolator epoch
        assert(_frameTime > 0);
        _epochPlayed += _frameTime;
        _animationGap += _frameTime;

        _frame++;

        // maybe print performance data
        totalStopwatch.reset();
        if (_timekeeper)
        {
            if (auto aggregation = _timekeeper->fetch(); aggregation)
            {
                MINIRE_INFO("{}", instrumentation::tabulate<double>(*aggregation));
            }
        }

        // ensure that frame was rendered fine (TODO: maybe move this to Rasterizer?)
        if (!kDebug)
        {
            if (opengl::isInPedanticMode())
            {
                _pedanticGlCounter++;
                if (_pedanticGlCounter > 1000)
                {
                    opengl::setGlErrorCheckMode(false);
                    MINIRE_INFO("Pendatic mode deactivated after {} frames",
                                _pedanticGlCounter);
                    _pedanticGlCounter = 0;
                }
            }
            else if (opengl::havePendedGlError())
            {
                opengl::setGlErrorCheckMode(true);
                MINIRE_ERROR("Unknown GL error detected! Pendatic mode is activated");
            }
        }
    }
}
