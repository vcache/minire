#pragma once

#include <minire/models/mouse-button.hpp>

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>

#include <cassert>
#include <cstdint>
#include <string>

class SDL_Window;
class SDL_Renderer;
class SDL_Texture;
union SDL_Event;
class SDL_WindowEvent;

namespace minire::sdl
{
    class Application
    {
    public:
        Application(int width, int height,
                    std::string const & title);
        virtual ~Application();

        void run();

    protected:
        virtual void onRender();
        virtual void onResize(size_t width, size_t height);
        virtual void onMouseWheel(int dx, int dy);
        virtual void onMouseMove(int absX, int absY, int relX, int relY,
                                 bool left, bool middle, bool right, bool x1, bool x2);
        virtual void onMouseDown(int x, int y, bool doubleClick, models::MouseButton);
        virtual void onMouseUp(int x, int y, bool doubleClick, models::MouseButton);
        virtual void onTextInput(std::string);

        // https://wiki.libsdl.org/SDL_Keycode
        // https://wiki.libsdl.org/SDL_Keymod
        virtual void onKeyUp(::SDL_Keycode, ::SDL_Scancode, uint16_t mod);
        virtual void onKeyDown(::SDL_Keycode, ::SDL_Scancode, uint16_t mod);

        virtual void onFps(size_t fps, double mft);

    protected:
        bool grabMouse(bool const grab);

    protected:
        uint32_t ticks() const { return _frameTicks; } // milliseconds, msec
        size_t width() const { return _width; }
        size_t height() const { return _height; }
        void stop() { _working = false; }

        SDL_Window * window() const { return _window; }

    private:
        void handle(SDL_Event const &);
        void handle(SDL_WindowEvent const &);
        void handleResize(int w, int h);

    private:
        SDL_Window * _window;
        size_t       _width;
        size_t       _height;
        std::string  _title;
        uint32_t     _frameTicks;
        bool         _working;
    };
}
