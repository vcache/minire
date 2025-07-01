#include <minire/sdl/application.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/fps-counter.hpp>

#include <SDL2/SDL.h>

#include <fmt/format.h>

namespace minire::sdl
{
    Application::Application(int width, int height,
                             std::string const & title)
        : _window(nullptr)
        , _width(width)
        , _height(height)
        , _title(title)
        , _working(true)
    {
        MINIRE_INVARIANT(width > 0 && height > 0,
                         "bad window size: {}x{}", width, height);

        try
        {
            if (::SDL_Init(SDL_INIT_VIDEO) != 0)
            {
                MINIRE_THROW("SDL_Init failed: {}", SDL_GetError());
            }

            _window = ::SDL_CreateWindow(
                title.c_str(),
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                width, height,
                SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
            if (!_window)
            {
                MINIRE_THROW("SDL_CreateWindow failed: {}", ::SDL_GetError());
            }
        }
        catch(...)
        {
            ::SDL_Quit();
            throw;
        }
    }
    
    Application::~Application()
    {
        if (_window) ::SDL_DestroyWindow(_window);
        _window = nullptr;
        ::SDL_Quit();
    }

    void Application::onRender() {}

    void Application::onResize(size_t, size_t) {}

    void Application::onMouseWheel(int, int) {}

    void Application::onMouseMove(int, int, int, int,
                             bool, bool, bool, bool, bool) {}

    void Application::onMouseDown(int, int, bool, models::MouseButton) {}

    void Application::onMouseUp(int, int, bool, models::MouseButton) {}
        
    void Application::onTextInput(std::string) {}

    void Application::onKeyUp(::SDL_Keycode, ::SDL_Scancode, uint16_t) {}
    
    void Application::onKeyDown(::SDL_Keycode, ::SDL_Scancode, uint16_t) {}

    void Application::onFps(size_t, double) {}

    bool Application::grabMouse(bool const grab)
    {
        ::SDL_SetWindowGrab(_window, grab ? SDL_TRUE : SDL_FALSE);
        ::SDL_ShowCursor(grab ? SDL_DISABLE : SDL_ENABLE);

        if (0 != ::SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE))
        {
            return false;
        }

        ::SDL_PumpEvents();
        ::SDL_FlushEvent(SDL_MOUSEMOTION);

        return true;
    }

    void Application::handleResize(int w, int h)
    {
        MINIRE_INVARIANT(w > 0 && h > 0,
                         "bad window size: {}x{}", w, h);
        _width = w;
        _height = h;
        onResize(w, h);
    }

    void Application::handle(SDL_WindowEvent const & e)
    {
        switch(e.event)
        {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED:
                handleResize(e.data1, e.data2);
                break;
        }
    }    

    void Application::handle(SDL_Event const & e)
    {
        // https://wiki.libsdl.org/SDL_Event
        switch(e.type)
        {
            case SDL_QUIT:
                _working = false;
                break;

            case SDL_WINDOWEVENT:
                handle(e.window);
                break;

            case SDL_KEYDOWN:
                onKeyDown(e.key.keysym.sym, e.key.keysym.scancode, e.key.keysym.mod);
                break;

            case SDL_KEYUP:
                onKeyUp(e.key.keysym.sym, e.key.keysym.scancode, e.key.keysym.mod);
                break;

            case SDL_MOUSEMOTION:
                onMouseMove(e.motion.x,
                            e.motion.y,
                            e.motion.xrel,
                            e.motion.yrel,
                            e.motion.state & SDL_BUTTON_LMASK,
                            e.motion.state & SDL_BUTTON_MMASK,
                            e.motion.state & SDL_BUTTON_RMASK,
                            e.motion.state & SDL_BUTTON_X1MASK,
                            e.motion.state & SDL_BUTTON_X2MASK);
                break;

            case SDL_MOUSEBUTTONDOWN:
                onMouseDown(e.button.x,
                            e.button.y,
                            e.button.clicks > 1,
                            models::mouseButtonFromSdl(e.button.button));
                break;

            case SDL_MOUSEBUTTONUP:
                onMouseUp(e.button.x,
                          e.button.y,
                          e.button.clicks > 1,
                          models::mouseButtonFromSdl(e.button.button));
                break;

            case SDL_MOUSEWHEEL:
                onMouseWheel(e.wheel.x, e.wheel.y);
                break;

            case SDL_TEXTINPUT:
                onTextInput(std::string(e.text.text));
                break;

            default:
                MINIRE_DEBUG("Unhandled SDL event: {:#x}", e.type);
        }
    }

    void Application::run()
    {
        utils::FpsCounter fpsCounter(2);
        while(_working)
        {
            _frameTicks = SDL_GetTicks();

            // events handling
            ::SDL_Event event;
            while (::SDL_PollEvent(&event))
            {
                handle(event);
            }

            // paint frame
            onRender();

            // count FPS
            if (auto fps = fpsCounter.registerFrame(); fps)
            {
                std::string title = fmt::format("[{}  fps, mft = {} ms]: {}",
                                                fps->first, fps->second, _title);
                ::SDL_SetWindowTitle(_window, title.c_str());
                onFps(fps->first, fps->second);
            }
        }
    }

}
