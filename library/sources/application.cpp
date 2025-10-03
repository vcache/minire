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
#include <utils/overloaded.hpp>

#include <fmt/format.h>

#include <algorithm>

namespace minire
{
    // TODO: read them from Camera
    static const float kNear = 0.001f;
    static const float kFar = 1000.0f;

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
        , _scene(std::make_unique<Scene>(*_rasterizer))
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
        _frameEnd = 0;

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

    Application::~Application()
    {
        // explicitly destroy controller until everything else start to tear down
        if (_controller)
        {
            _controller->shutdown();
            _controller.reset();
        }
    }

    void Application::setSquashAdditiveEvents(bool enabled)
    {
        _squashAdditiveEvents = enabled;
    }

    bool Application::squashAdditiveEvents() const
    {
        return _squashAdditiveEvents;
    }

    template<typename Event, typename... Args>
    void Application::postEvent(Args && ... args)
    {
        _applicationEvents.emplace_back(Event(std::forward<Args>(args)...));
        if (_controller && _controller->lowLatencyInput())
        {
            pushPendedEvents();
        }
    }

    template<typename Event>
    Event * Application::findEventToSquash()
    {
        if (_squashAdditiveEvents && !_applicationEvents.empty())
        {
            return std::get_if<Event>(&_applicationEvents.back());
        }
        return nullptr;
    }

    void Application::onKeyUp(::SDL_Keycode key, ::SDL_Scancode code, uint16_t mod)
    {
        postEvent<events::application::OnKeyUp>(key, code, mod);
    }

    void Application::onKeyDown(::SDL_Keycode key, ::SDL_Scancode code, uint16_t mod)
    {
        postEvent<events::application::OnKeyDown>(key, code, mod);
    }

    void Application::onTextInput(std::string str)
    {
        if (!str.empty())
        {
            postEvent<events::application::OnTextInput>(std::move(str));
        }
    }

    void Application::onMouseWheel(int dx, int dy)
    {
        using namespace events::application;
        if (auto * last = findEventToSquash<OnMouseWheel>(); last)
        {
            last->_dx += dx;
            last->_dy += dy;
        }
        else
        {
            postEvent<OnMouseWheel>(dx, dy);
        }
    }

    void Application::onMouseMove(int absX, int absY, int relX, int relY,
                                  bool left, bool middle, bool right,
                                  bool x1, bool x2)
    {
        using namespace events::application;

        _mouseX = absX;
        _mouseY = absY;

        if (auto * last = findEventToSquash<OnMouseMove>();
            last && last->_left == left && last->_middle == middle &&
            last->_right == right && last->_x1 == x1 && last->_x2 == x2)
        {
            last->_absX = absX;
            last->_absY = absY;
            last->_relX += relX;
            last->_relY += relY;
        }
        else
        {
            postEvent<OnMouseMove>(absX, absY, relX, relY, left, middle, right, x1, x2);
        }
    }

    void Application::onMouseDown(int x, int y, bool doubleClick,
                                  models::MouseButton mouseButton)
    {
        _mouseX = x;
        _mouseY = y;
        postEvent<events::application::OnMouseDown>(
            x, y, mouseButton, doubleClick);
    }
    
    void Application::onMouseUp(int x, int y, bool doubleClick,
                                models::MouseButton mouseButton)
    {
        _mouseX = x;
        _mouseY = y;
        postEvent<events::application::OnMouseUp>(
            x, y, mouseButton, doubleClick);
    }

    void Application::onResize(size_t width, size_t height)
    {
        MINIRE_GL(glViewport, 0, 0, width, height);

        float const fWidth = static_cast<float>(width);
        float const fHeight = static_cast<float>(height);

        // required for a Projection matrix
        _scene->setViewport(width, height);

        // projection for 2D gui
        _rasterizer->setScreenSize(fWidth, fHeight);

        // send event to controller
        if (auto * last = findEventToSquash<events::application::OnResize>(); last)
        {
            last->_width = width;
            last->_height = height;
        }
        else
        {
            postEvent<events::application::OnResize>(width, height);
        }
    }

    void Application::onFps(size_t fps, double mft)
    {
        postEvent<events::application::OnFps>(fps, mft, _frame);
    }

    void Application::maybeIssueRayCaster()
    {
        assert(_scene);
        if (_rayCasterEnabled && _rayCasterLastEpoch < _epochNumber)
        {
            scene::Viewpoint const & vp = _scene->viewpoint();
            if (size_t const vpRevision = vp.revision();
                _rayCasterRevision < vpRevision)
            {
                auto rayCaster = std::make_shared<utils::RayCaster>(
                    vp.width(), vp.height(), vp.view(), vp.projection());
                postEvent<events::application::OnRayCaster>(rayCaster);
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

    void Application::handle(events::controller::Quit const &)
    {
        MINIRE_THROW("TODO: not implemented");
    }

    void Application::handle(events::controller::MouseGrab const & e)
    {
        MINIRE_DEBUG("mouse grab event = {}", e._grab);
        grabMouse(e._grab);
    }

    void Application::handle(events::controller::DebugDrawsUpdate const & e)
    {
        _rasterizer->lines().update(e._linesBuffer);
    }

    void Application::handle(events::controller::SetInstrumentation const & e)
    {
        if (e._enabled)
        {
            enableInstrumentation();
        }
        else
        {
            disableInstrumentation();
        }
    }

    void Application::handle(events::controller::NewResourceLayer const & e)
    {
        _rasterizer->newResourceLayer(e._name);
        _contentManager.newLayer(e._name);
    }

    void Application::handle(events::controller::DisposeResourceLayer const & e)
    {
        _rasterizer->disposeResourceLayer(e._name);
        _contentManager.disposeLayer(e._name);
    }

    void Application::handle(events::controller::ContentManagerCleanup const & e)
    {
        _contentManager.cleanup(e._force);
    }

    void Application::handle(events::controller::CreateVertexBuffer const & e)
    {
        _rasterizer->vertexBuffers().create(e._id, e._vertexBuffer, e._override);
    }

    void Application::handle(events::controller::DisposeVertexBuffer const & e)
    {
        _rasterizer->vertexBuffers().dispose(e._id);
    }

    void Application::handle(events::controller::CreateSprite const & e)
    {
        _rasterizer->sprites().create(e._id, e._texture, e._source, e._position,
                                      e._dimensions, e._visible, e._zOrder);
    }

    void Application::handle(events::controller::ResizeSprite const & e)
    {
        _rasterizer->sprites().resize(e._id, e._dimensions);
    }

    void Application::handle(events::controller::MoveSprite const & e)
    {
        _rasterizer->sprites().move(e._id, e._position);
    }

    void Application::handle(events::controller::SetSpriteVisible const & e)
    {
        _rasterizer->sprites().visible(e._id, e._visible);
    }

    void Application::handle(events::controller::RemoveSprite const & e)
    {
        _rasterizer->sprites().remove(e._id);
    }

    void Application::handle(events::controller::BulkSetSpriteZOrders const & e)
    {
        for(std::pair<std::string, size_t> const & i : e._items)
        {
            MINIRE_DEBUG("setting Z for sprite \"{}\" to {}", i.first, i.second);
            _rasterizer->sprites().setZOrder(i.first, i.second);
        }
    }

    void Application::handle(events::controller::CreateLabel const & e)
    {
        rasterizer::Label & label = _rasterizer->labels().allocate(e._id, e._text, e._zOrder, e._visible);
        label.setFontFace(e._fontFace, _contentManager);
        label.setPosition(e._position);
    }

    void Application::handle(events::controller::MoveLabel const & e)
    {
        _rasterizer->labels().get(e._id).setPosition(e._position);
    }

    void Application::handle(events::controller::SetLabelVisible const & e)
    {
        _rasterizer->labels().get(e._id).setVisible(e._visible);
    }

    void Application::handle(events::controller::SetLabelFontFace const & e)
    {
        _rasterizer->labels().get(e._id).setFontFace(e._fontFace, _contentManager);
    }

    void Application::handle(events::controller::SetLabelClipping const & e)
    {
        _rasterizer->labels().get(e._id).setMaxSize(e._maxSize);
    }

    void Application::handle(events::controller::SetLabelText const & e)
    {
        _rasterizer->labels().get(e._id).setText(e._text);
    }

    void Application::handle(events::controller::RemoveLabel const & e)
    {
        _rasterizer->labels().deallocate(e._id);
    }

    void Application::handle(events::controller::BulkSetLabelZOrders const & e)
    {
        for(std::pair<std::string, size_t> const & i : e._items)
        {
            MINIRE_DEBUG("setting Z for label \"{}\" to {}", i.first, i.second);
            _rasterizer->labels().get(i.first).setZOrder(i.second);
        }
    }

    void Application::handle(events::controller::StartTextInput const &)
    {
        ::SDL_StartTextInput();
    }
    
    void Application::handle(events::controller::StopTextInput const &)
    {
        ::SDL_StopTextInput();
    }

    void Application::handle(events::controller::SceneReset const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneDispose const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneActivateCamera const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneSetAmbientLight const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneNewNode const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneNewFromSource const & e)
    {
        scene::instantiateGltf(*_scene, e, _contentManager);
    }

    void Application::handle(events::controller::SceneNewMesh const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneNewPointLight const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneNewPerspectiveCamera const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneNewOrthographicCamera const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneSetMeshAmbientLight const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneSetParent const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneSetVisibility const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneSetTransform const & e)
    {
        _scene->handle(e, _epochNumber);
    }

    void Application::handle(events::controller::SceneSetPointLight const & e)
    {
        _scene->handle(e, _epochNumber);
    }

    void Application::handle(events::controller::SceneSetPerspectiveCamera const & e)
    {
        _scene->handle(e, _epochNumber);
    }

    void Application::handle(events::controller::SceneSetOrthographicCamera const & e)
    {
        _scene->handle(e, _epochNumber);
    }

    void Application::handle(events::controller::SceneNewAnimationSet const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::ScenePlayAnimation const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SceneStopAnimation const & e)
    {
        _scene->handle(e);
    }

    void Application::handle(events::controller::SetRayCaster const & e)
    {
        _rayCasterEnabled = e._enabled;
        if (_rayCasterEnabled)
        {
            _rayCasterRevision = 0;
        }
    }

    void Application::handle(BasicController::Batch const & batch)
    {
#       ifndef NDEBUG
        if (batch._events.size() > 1000)
        {
            MINIRE_DEBUG("Got {} event inm controller's batch",
                         batch._events.size());
        }
#       endif

        for(events::Controller const & event: batch._events)
        {
            // TODO: it might throw
            std::visit([this](auto const & e) { handle(e); }, event);
        }
    }

    void Application::pushPendedEvents()
    {
        // TODO: does it have to be called when the queue is empty?
        size_t const pendedEvents = _applicationEvents.size(); // TODO: reserve max or p99
        _controller->push(std::move(_applicationEvents));
        _applicationEvents = events::ApplicationQueue();
        _applicationEvents.reserve(pendedEvents);
    }

    void Application::onRender()
    {
        assert(_controller);

        auto totalStopwatch =
            _timekeeper ? std::make_unique<instrumentation::Stopwatch<>>("total", _timekeeper)
                        : std::unique_ptr<instrumentation::Stopwatch<>>();

        // maybe issue a ray caster
        {
            instrumentation::Stopwatch<> stopwatch("ray-caster", _timekeeper);
            maybeIssueRayCaster();
        }

        // notify logic thread about new events
        {
            instrumentation::Stopwatch<> stopwatch("controller-notify", _timekeeper);
            pushPendedEvents();
        }

        // fetch and handle events from controller if any
        {
            instrumentation::Stopwatch<> stopwatch("batch-fetching", _timekeeper);
            BasicController::BatchQueue batchQueue = _controller->pull();
            std::move(batchQueue.begin(), batchQueue.end(),
                      std::back_inserter(_controllerEvents));
        }

        bool performLerp = false;
        bool newEpochStarted = false;
        if (!_controllerEvents.empty())
        {
            instrumentation::Stopwatch<> stopwatch("events-handling", _timekeeper);
            if (_batchPlayed < 0)
            {
                // very first batch and very slow controller case
                handle(_controllerEvents[0]);
                _batchPlayed = 0;
                performLerp = true;
            }
            else if (_batchPlayed < _controllerEvents[0]._duration)
            {
                // middle of a batch
                assert(_batchPlayed >= 0);
                assert(_controllerEvents[0]._duration != 0);
                performLerp = true;
            }
            else
            {
                assert(_batchPlayed >= _controllerEvents[0]._duration);

                // purge currently played batch
                _batchPlayed -= _controllerEvents[0]._duration;
                _controllerEvents.erase(_controllerEvents.begin());

                // fast-forward hidden ones (they will be invisible,
                // but they might containt important events)
                while(!_controllerEvents.empty() &&
                      _batchPlayed >= _controllerEvents[0]._duration)
                {
                    handle(_controllerEvents[0]);
                    _batchPlayed -= _controllerEvents[0]._duration;
                    _controllerEvents.erase(_controllerEvents.begin());
                }

                _epochNumber++;
                newEpochStarted = true;

                if (_controllerEvents.empty())
                {
                    _batchPlayed = -1;
                }
                else
                {
                    assert(_batchPlayed >= 0);
                    handle(_controllerEvents[0]);
                    performLerp = true;
                }
            }
        }

        if (newEpochStarted)
        {
            instrumentation::Stopwatch<> stopwatch("animation-advance", _timekeeper);
            performLerp |= _scene->advanceAnimations(_animationGap, _epochNumber);
            _animationGap = 0;
        }

        if (performLerp)
        {
            instrumentation::Stopwatch<> stopwatch("scene-lerping", _timekeeper);
            double const weight = _batchPlayed / _controllerEvents[0]._duration;
            _scene->lerp(weight, _epochNumber);
        }

        {
            instrumentation::Stopwatch<> stopwatch("scene-revalidation", _timekeeper);
            _scene->revalidateNodes();
        }

        // draw a frame
        {
            instrumentation::Stopwatch<> stopwatch("scene-rendering", _timekeeper);
            MINIRE_GL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            _rasterizer->draw(*_scene);
            ::SDL_GL_SwapWindow(window());
        }

        // calc frame time
        _frameEnd = utils::uNow();
        double frameTime = double(_frameEnd - _frameBegin) / 1000000.0; // sec
        frameTime = std::min(1.0, frameTime); // prevent from going haywire
        _frameBegin = _frameEnd;

        // advance interpolator epoch
        assert(frameTime > 0);
        _batchPlayed += frameTime;
        _animationGap += frameTime;

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
