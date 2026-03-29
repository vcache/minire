#include <minire/sdl/application.hpp>

#include <utils/fps-counter.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <fmt/format.h>
#include <SDL2/SDL.h>

namespace minire::sdl
{
    namespace
    {
        template<typename T>
        struct SdlDeleter
        {
            void operator()(T * p) const { ::SDL_free(p); }
        };

        template<typename T>
        using SdlUptr = std::unique_ptr<T, SdlDeleter<T>>;

        ::SDL_SystemCursor toSdlCursor(models::SystemCursor const systemCursor)
        {
            switch(systemCursor)
            {
                case models::SystemCursor::kArrow:      return SDL_SYSTEM_CURSOR_ARROW;
                case models::SystemCursor::kIbeam:      return SDL_SYSTEM_CURSOR_IBEAM;
                case models::SystemCursor::kWait:       return SDL_SYSTEM_CURSOR_WAIT;
                case models::SystemCursor::kCrosshair:  return SDL_SYSTEM_CURSOR_CROSSHAIR;
                case models::SystemCursor::kWaitArrow:  return SDL_SYSTEM_CURSOR_WAITARROW;
                case models::SystemCursor::kSizeNWSE:   return SDL_SYSTEM_CURSOR_SIZENWSE;
                case models::SystemCursor::kSizeNESW:   return SDL_SYSTEM_CURSOR_SIZENESW;
                case models::SystemCursor::kSizeWE:     return SDL_SYSTEM_CURSOR_SIZEWE;
                case models::SystemCursor::kSizeNS:     return SDL_SYSTEM_CURSOR_SIZENS;
                case models::SystemCursor::kSizeAll:    return SDL_SYSTEM_CURSOR_SIZEALL;
                case models::SystemCursor::kNo:         return SDL_SYSTEM_CURSOR_NO;
                case models::SystemCursor::kHand:       return SDL_SYSTEM_CURSOR_HAND;
            }

            MINIRE_THROW("unexpected SystemCursor value: {}",
                         static_cast<int>(systemCursor));
        }
    }

    Application::ColorCursor::ColorCursor(models::Image::Sptr const & image,
                                          int hotX, int hotY)
        : _hotX(hotX)
        , _hotY(hotY)
        , _image(image)
    {
        // check preconditions

        MINIRE_INVARIANT(image, "no image provided for a cursor");
        MINIRE_INVARIANT(image->_width > 0 && image->_height > 0,
                         "bad image size: {}x{}", image->_width, image->_height);
        MINIRE_INVARIANT(image->_data, "no pixel data");
        MINIRE_INVARIANT(!image->_signed, "signed components aren't supported");
        MINIRE_INVARIANT(image->_depth == models::Image::Depth::k8,
                         "unsupported image component depth: {}",
                         static_cast<int>(image->_depth));

        // build a Surface

        switch(image->_format)
        {
            using enum models::Image::Format;

            case kRGB:
                _surface.reset(::SDL_CreateRGBSurfaceFrom(image->_data, image->_width, image->_height,
                                                          8 * image->bytesInPixel(), image->bytesInLine(),
                                                          0x0000'00FF,  // R
                                                          0x0000'FF00,  // G
                                                          0x00FF'0000,  // B
                                                          0x0000'0000));// A
                break;

            case kRGBA:
                _surface.reset(::SDL_CreateRGBSurfaceFrom(image->_data, image->_width, image->_height,
                                                          8 * image->bytesInPixel(), image->bytesInLine(),
                                                          0x0000'00FF,  // R
                                                          0x0000'FF00,  // G
                                                          0x00FF'0000,  // B
                                                          0xFF00'0000));// A
                break;

            default:
                MINIRE_THROW("unsupported image format: {}",
                             static_cast<int>(image->_format));
        }

        MINIRE_INVARIANT(_surface, "failed to create SDL Surface: {}",
                         ::SDL_GetError());

        // Build a cursor

        _cursor.reset(::SDL_CreateColorCursor(_surface.get(), _hotX, _hotY));
        MINIRE_INVARIANT(_cursor, "failed to create SDL Color Cursor: {}",
                         ::SDL_GetError());
    }

    Application::ColorCursor::~ColorCursor()
    {
        _cursor.reset();
        _surface.reset();
        _image.reset();
    }

    Application::Application(int width, int height,
                             std::string const & title,
                             models::MsaaParams const & msaaParams)
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

            MINIRE_INVARIANT(0 == ::SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, msaaParams._buffers),
                             "SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, {}): {}",
                             msaaParams._buffers, ::SDL_GetError());
            MINIRE_INVARIANT(0 == ::SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaaParams._samples),
                             "SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, {}): {}",
                             msaaParams._buffers, ::SDL_GetError());

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
        if (_cursor)
        {
            ::SDL_Cursor * defaultCursor = ::SDL_GetDefaultCursor();
            ::SDL_SetCursor(defaultCursor);
            _cursor.reset();
        }

        if (_window) ::SDL_DestroyWindow(_window);
        _window = nullptr;
        ::SDL_Quit();
    }

    void Application::onRender() {}

    void Application::onResize(size_t, size_t) {}

    void Application::onMouseWheel(int, int, uint32_t, ::SDL_Keymod) {}

    void Application::onMouseMove(int, int, int, int,
                                  bool, bool, bool, bool, bool) {}

    void Application::onMouseDown(int, int, bool, models::MouseButton) {}

    void Application::onMouseUp(int, int, bool, models::MouseButton) {}

    void Application::onTextInput(std::string const &) {}

    void Application::onKeyUp(::SDL_Keycode, ::SDL_Scancode, uint16_t) {}

    void Application::onKeyDown(::SDL_Keycode, ::SDL_Scancode, uint16_t) {}

    void Application::onFps(size_t, double) {}

    bool Application::setMouseMode(bool const windowGrab,
                                   bool const showCursor,
                                   bool const relativeMode)
    {
        ::SDL_SetWindowGrab(_window, windowGrab ? SDL_TRUE : SDL_FALSE);

        ::SDL_ShowCursor(showCursor ? SDL_ENABLE : SDL_DISABLE);
        ::SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_CURSOR_VISIBLE, showCursor ? "1" : "0");

        if (0 != ::SDL_SetRelativeMouseMode(relativeMode ? SDL_TRUE : SDL_FALSE))
        {
            return false;
        }

        ::SDL_PumpEvents();
        ::SDL_FlushEvent(SDL_MOUSEMOTION);

        return true;
    }

    void Application::setSystemCursor(models::SystemCursor const systemCursor)
    {
        if (SDL_Cursor * newCursorPtr = ::SDL_CreateSystemCursor(toSdlCursor(systemCursor));
            newCursorPtr)
        {
            SdlCursorUptr newCursor(newCursorPtr);
            ::SDL_SetCursor(newCursor.get());
            _cursor = std::move(newCursor);
        }
        else
        {
            MINIRE_WARNING("failed to create system cursor: {}", ::SDL_GetError());
        }
    }

    void Application::setColorCursor(ColorCursor::Sptr const & colorCursor)
    {
        if (colorCursor && colorCursor->_cursor)
        {
            ::SDL_SetCursor(colorCursor->_cursor.get());
        }
    }

    void Application::setClipboardText(std::string const & text) const
    {
        if (int const result = ::SDL_SetClipboardText(text.c_str());
            0 != result)
        {
            MINIRE_ERROR("Failed to set clipboard text: {}",
                         ::SDL_GetError());
        }
    }

    std::string Application::clipboardText() const
    {
        std::string clipboardText;
        if (::SDL_HasClipboardText())
        {
            if (SdlUptr<char> text(::SDL_GetClipboardText()); text)
            {
                clipboardText = text.get();
            }
            else
            {
                MINIRE_ERROR("failed to emit clipboard update: {}", ::SDL_GetError());
            }
        }
        return clipboardText;
    }

    void Application::setPrimarySelection(std::string const & text) const
    {
        if (int const result = ::SDL_SetPrimarySelectionText(text.c_str());
            0 != result)
        {
            MINIRE_ERROR("Failed to set primary selection text: {}",
                         ::SDL_GetError());
        }
    }

    std::string Application::primarySelection() const
    {
        std::string primarySelection;
        if (::SDL_HasPrimarySelectionText())
        {
            if (SdlUptr<char> text(::SDL_GetPrimarySelectionText()); text)
            {
                primarySelection = text.get();
            }
            else
            {
                MINIRE_ERROR("failed to emit primary selection update: {}", ::SDL_GetError());
            }
        }
        return primarySelection;
    }

    void Application::startTextInput() const
    {
        ::SDL_StartTextInput();
    }

    void Application::stopTextInput() const
    {
        ::SDL_StopTextInput();
    }

    void Application::handleResize(int w, int h)
    {
        MINIRE_INVARIANT(w > 0 && h > 0,
                         "bad window size: {}x{}", w, h);
        _width = w;
        _height = h;
        onResize(w, h);
    }

    void Application::handleEvent(SDL_WindowEvent const & e)
    {
        switch(e.event)
        {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED:
                handleResize(e.data1, e.data2);
                break;
        }
    }

    void Application::handleEvent(SDL_Event const & e)
    {
        // https://wiki.libsdl.org/SDL_Event
        switch(e.type)
        {
            case SDL_QUIT:
                _working = false;
                break;

            case SDL_WINDOWEVENT:
                handleEvent(e.window);
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
                onMouseWheel(e.wheel.x, e.wheel.y, e.wheel.direction,
                             ::SDL_GetModState());
                break;

            case SDL_TEXTINPUT:
                onTextInput(std::string(e.text.text));
                break;

            default:
                MINIRE_DEBUG("Unhandled SDL event: {:#x}", e.type);
        }
    }

    void Application::onStart() {}

    void Application::onEnd() {}

    // TODO: add FPS limiter
    void Application::run()
    {
        utils::FpsCounter fpsCounter(2);
        onStart();
        while(_working)
        {
            _frameTicks = SDL_GetTicks();

            // events handling
            ::SDL_Event event;
            while (::SDL_PollEvent(&event))
            {
                handleEvent(event);
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
        onEnd();
    }

}
