#include <minire/application.hpp>

#include <minire/logging.hpp>
#include <minire/utils/unow.hpp>
#include <opengl.hpp>

// TODO: implement an interpolation between a logical frame and a gpu frame

namespace minire
{
    // TODO: move them into parameters
    static const float kNear = 0.1f;
    static const float kFar = 100.0f;

    Application::Application(int width, int height,
                             std::string const & title,
                             content::Manager & contentManager)
        : sdl::GlApplication(width, height, title)
        , _contentManager(contentManager)
        , _gpuRender(contentManager)
        , _scene(_gpuRender)
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

    template<typename Event, typename... Args>
    void Application::postEvent(Args && ... args)
    {
        _applicationEvents.push(Event(std::forward<Args>(args)...));
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
        postEvent<events::application::OnMouseWheel>(dx, dy);
    }

    void Application::onMouseMove(int absX, int absY, int relX, int relY,
                                  bool left, bool middle, bool right,
                                  bool x1, bool x2)
    {
        postEvent<events::application::OnMouseMove>(
            absX, absY, relX, relY, left, middle, right, x1, x2);
    }

    void Application::onMouseDown(int x, int y, bool doubleClick,
                                  models::MouseButton mouseButton)
    {
        postEvent<events::application::OnMouseDown>(
            x, y, mouseButton, doubleClick);
    }
    
    void Application::onMouseUp(int x, int y, bool doubleClick,
                                models::MouseButton mouseButton)
    {
        postEvent<events::application::OnMouseUp>(
            x, y, mouseButton, doubleClick);
    }

    void Application::onResize(size_t width, size_t height)
    {
        MINIRE_GL(glViewport, 0, 0, width, height);

        constexpr int kMode = 0;    // TODO: why so hardcoded?

        float const fWidth = static_cast<float>(width);
        float const fHeight = static_cast<float>(height);

        // TODO: zoom in/out by changin FOV

        glm::mat4 pmat;
        switch(kMode)
        {
            case 0:
                pmat = glm::perspective(
                    glm::radians(45.0f), fWidth / fHeight, kNear, kFar);
                break;
            
            case 1:
                pmat = glm::perspectiveFov(
                    glm::radians(120.0f), fWidth, fHeight, kNear, kFar);
                break;
            
            case 2: {
                float const ratio = fWidth / fHeight;
                float const scale = 10.0f;
                pmat = glm::ortho(-scale, scale,
                                  -scale * ratio, scale * ratio,
                                  kNear, kFar);
                break;
            }

            case 3: {
                //pmat = glm::ortho(0.0f, fWidth, 0.0f, fHeight, kNear, kFar);
                float const scale = .01f;
                pmat = glm::ortho(-fWidth/2 * scale,
                                  fWidth/2 * scale,
                                  -fHeight/2 * scale,
                                  fHeight/2 * scale,
                                  kNear, kFar);
                break;
            }

            default: MINIRE_THROW("bad projection mode: {}", kMode);
        }

        _viewpoint.setProjection(pmat, fWidth, fHeight);

        // projection for 2D gui
        _gpuRender.setScreenSize(fWidth, fHeight);

        // send event to controller
        postEvent<events::application::OnResize>(width, height);
    }

    void Application::onFps(size_t fps, double mft)
    {
        postEvent<events::application::OnFps>(fps, mft, _frame);
    }

    void Application::handle(events::controller::Quit const &)
    {
        MINIRE_THROW("TODO: not implemented");
    }

    void Application::handle(events::controller::NewEpoch const & e)
    {
        _epochInterpolator.newEpoch(e._epochNumber, e._epochLength);
    }

    void Application::handle(events::controller::MouseGrab const & e)
    {
        MINIRE_DEBUG("mouse grab event = {}", e._grab);
        grabMouse(e._grab);
    }

    void Application::handle(events::controller::DebugDrawsUpdate const & e)
    {
        _gpuRender.lines().update(e._linesBuffer);
    }

    void Application::handle(events::controller::CreateSprite const & e)
    {
        _gpuRender.sprites().create(e._id, e._texture, e._tile, e._position,
                                    e._visible, e._z);
    }

    void Application::handle(events::controller::CreateNinePatch const & e)
    {
        _gpuRender.sprites().create(e._id, e._texture, e._tile, e._position,
                                    e._dimensions, e._visible, e._z);
    }

    void Application::handle(events::controller::ResizeNinePatch const & e)
    {
        _gpuRender.sprites().resize(e._id, e._dimensions);
    }

    void Application::handle(events::controller::MoveSprite const & e)
    {
        _gpuRender.sprites().move(e._id, e._position);
    }

    void Application::handle(events::controller::VisibleSprite const & e)
    {
        _gpuRender.sprites().visible(e._id, e._visible);
    }

    void Application::handle(events::controller::RemoveSprite const & e)
    {
        _gpuRender.sprites().remove(e._id);
    }

    void Application::handle(events::controller::BulkSetSpriteZOrders const & e)
    {
        for(std::pair<std::string, size_t> const & i : e._items)
        {
            MINIRE_DEBUG("setting Z for sprite \"{}\" to {}", i.first, i.second);
            _gpuRender.sprites().setZOrder(i.first, i.second);
        }
    }

    void Application::handle(events::controller::CreateLabel const & e)
    {
        _gpuRender.labels().allocate(e._id, e._z, e._visible);
    }

    void Application::handle(events::controller::ResizeLabel const & e)
    {
        _gpuRender.labels().get(e._id).resize(e._rows, e._cols);
    }

    void Application::handle(events::controller::MoveLabel const & e)
    {
        _gpuRender.labels().get(e._id).setPosition(e._x, e._y);
    }

    void Application::handle(events::controller::SetCharLabel const & e)
    {
        _gpuRender
            .labels()
            .get(e._id)
            .at(e._row, e._col)
            .set(e._format, e._char);
    }

    void Application::handle(events::controller::SetSymbolLabel const & e)
    {
        _gpuRender
            .labels()
            .get(e._id)
            .at(e._row, e._col) = e._symbol;
    }
    
    void Application::handle(events::controller::UnsetCharLabel const & e)
    {
        _gpuRender
            .labels()
            .get(e._id)
            .at(e._row, e._col)
            .unset();
    }

    void Application::handle(events::controller::SetStringLabel const & e)
    {
        _gpuRender.labels().get(e._id).set(e._row, e._col, e._string);
    }

    void Application::handle(events::controller::SetLabelCursor const & e)
    {
        _gpuRender.labels().get(e._id).setCursor(e._col, e._row);
    }

    void Application::handle(events::controller::UnsetLabelCursor const & e)
    {
        _gpuRender.labels().get(e._id).unsetCursor();
    }

    void Application::handle(events::controller::SetLabelVisible const & e)
    {
        _gpuRender.labels().get(e._id).setVisible(e._visible);
    }

    void Application::handle(events::controller::SetLabelFonts const & e)
    {
        _gpuRender.labels().get(e._id).setFont(e._fontName,
                                               _contentManager);
    }

    void Application::handle(events::controller::RemoveLabel const & e)
    {
        _gpuRender.labels().deallocate(e._id);
    }

    void Application::handle(events::controller::BulkSetLabelZOrders const & e)
    {
        for(std::pair<std::string, size_t> const & i : e._items)
        {
            MINIRE_DEBUG("setting Z for label \"{}\" to {}", i.first, i.second);
            _gpuRender.labels().get(i.first).setZOrder(i.second);
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
        _scene.handle(e);
    }

    void Application::handle(events::controller::SceneEmergeModel const & e)
    {
        _scene.handle(e);
    }

    void Application::handle(events::controller::SceneEmergePointLight const & e)
    {
        _scene.handle(e);
    }

    void Application::handle(events::controller::SceneUnmergeModel const & e)
    {
        _scene.handle(e);
    }

    void Application::handle(events::controller::SceneUnmergePointLight const & e)
    {
        _scene.handle(e);
    }

    void Application::handle(events::controller::SceneUpdateFpsCamera const & e)
    {
        _camera.update(_epochInterpolator.epochNumber(), e._fps);
        _cameraActive = true;
    }

    void Application::handle(events::controller::SceneUpdateModel const & e)
    {
        _scene.handle(_epochInterpolator.epochNumber(), e);
    }

    void Application::handle(events::controller::SceneSetSelectedModels const & e)
    {
        _scene.handle(e);
    }

    void Application::handle(events::controller::SceneUpdateLight const & e)
    {
        _scene.handle(_epochInterpolator.epochNumber(), e);
    }

    void Application::handle(events::ControllerQueue::Store const & events)
    {
        if (events.size() > 50)
        {
            MINIRE_DEBUG("Got {} events from controller", events.size());
        }

        for(events::Controller const & event: events)
        {
            std::visit([this](auto const & e) { handle(e); }, event);
        }
    }

    void Application::onRender()
    {
        // notify logic thread about new events
        _applicationEvents.finish();

        // fetch and handle events from controller if any
        assert(_controller);
        handle(_controller->events());

        float weight = _epochInterpolator.weight();
        size_t epoch = _epochInterpolator.epochNumber();

        // lerp camera
        if (_cameraActive)
        {
            _cameraActive = _camera.lerp(weight, epoch);
            models::FpsCamera const & camera = _camera.current();
            _viewpoint.setView(camera.view(), camera.position());
        }

        // lerp scene
        _scene.lerp(weight, epoch);

        // draw a frame
        MINIRE_GL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        _gpuRender.draw(_viewpoint, _scene);
        ::SDL_GL_SwapWindow(window());

        // calc frame time
        _frameEnd = utils::uNow();
        double frameTime = double(_frameEnd - _frameBegin) / 1000000.0; // sec
        frameTime = std::min(1.0, frameTime); // prevent from going haywire
        _frameBegin = _frameEnd;

        // advance interpolator epoch
        _epochInterpolator.accumulate(frameTime);

        _frame++;
    }
}