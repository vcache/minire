#pragma once

#include <minire/models/image.hpp>
#include <minire/models/mouse-button.hpp>
#include <minire/models/msaa-params.hpp>
#include <minire/models/system-cursor.hpp>

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_surface.h>

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

class SDL_Renderer;
class SDL_Texture;
class SDL_Window;
class SDL_WindowEvent;
union SDL_Event;

namespace minire::sdl
{
    class Application
    {
        struct SdlCursorDeleter
        {
            void operator()(SDL_Cursor * c) const { ::SDL_FreeCursor(c); }
        };

        using SdlCursorUptr = std::unique_ptr<SDL_Cursor, SdlCursorDeleter>;

        struct SdlSurfaceDeleter
        {
            void operator()(SDL_Surface * c) const { ::SDL_FreeSurface(c); }
        };

        using SdlSurfaceUptr = std::unique_ptr<SDL_Surface, SdlSurfaceDeleter>;

    public:
        Application(int width, int height,
                    std::string const & title,
                    models::MsaaParams const & = {});
        virtual ~Application();

        void run();

    protected:
        virtual void onStart();
        virtual void onEnd();

        virtual void onRender();
        virtual void onResize(size_t width, size_t height);
        virtual void onMouseWheel(int dx, int dy, uint32_t dir, ::SDL_Keymod);
        virtual void onMouseMove(int absX, int absY, int relX, int relY,
                                 bool left, bool middle, bool right, bool x1, bool x2);
        virtual void onMouseDown(int x, int y, bool doubleClick, models::MouseButton);
        virtual void onMouseUp(int x, int y, bool doubleClick, models::MouseButton);
        virtual void onTextInput(std::string const &);

        // https://wiki.libsdl.org/SDL_Keycode
        // https://wiki.libsdl.org/SDL_Keymod
        virtual void onKeyUp(::SDL_Keycode, ::SDL_Scancode, uint16_t mod);
        virtual void onKeyDown(::SDL_Keycode, ::SDL_Scancode, uint16_t mod);

        virtual void onFps(size_t fps, double mft);

    protected:
        class ColorCursor
        {
            ColorCursor(ColorCursor const &) = delete;
            ColorCursor(ColorCursor &&) = delete;
            ColorCursor& operator=(ColorCursor const &) = delete;
            ColorCursor& operator=(ColorCursor &&) = delete;

        public:
            explicit ColorCursor(models::Image::Sptr const &,
                                 int hotX, int hotY);
            ~ColorCursor();

            using Sptr = std::shared_ptr<ColorCursor>;

        private:
            int const           _hotX = 0;
            int const           _hotY = 0;
            models::Image::Sptr _image;
            SdlSurfaceUptr      _surface;
            SdlCursorUptr       _cursor;

            friend class Application;
        };

        bool setMouseMode(bool const windowGrab,
                          bool const showCursor,
                          bool const relativeMode);

        void setSystemCursor(models::SystemCursor const);
        void setColorCursor(ColorCursor::Sptr const &);

    protected:
        void setClipboardText(std::string const &) const;
        std::string clipboardText() const;

        void setPrimarySelection(std::string const &) const;
        std::string primarySelection() const;

    protected:
        void startTextInput() const;
        void stopTextInput() const;

    protected:
        uint32_t ticks() const { return _frameTicks; } // milliseconds, msec
        size_t width() const { return _width; }
        size_t height() const { return _height; }
        void stop() { _working = false; }

        SDL_Window * window() const { return _window; }

    private:
        void handleEvent(SDL_Event const &);
        void handleEvent(SDL_WindowEvent const &);
        void handleResize(int w, int h);

    private:
        SDL_Window  * _window;
        size_t        _width;
        size_t        _height;
        std::string   _title;
        uint32_t      _frameTicks;
        SdlCursorUptr _cursor;
        bool          _working;
    };
}
