#pragma once

#include <minire/models/mouse-button.hpp>

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>

#include <cstddef>
#include <string>

namespace minire::application
{
    struct OnResize
    {
        size_t _width;
        size_t _height;
    };

    struct OnKeyDown
    {
        ::SDL_Keycode  _key;
        ::SDL_Scancode _code;
        uint16_t       _mod;
    };

    struct OnKeyUp
    {
        ::SDL_Keycode  _key;
        ::SDL_Scancode _code;
        uint16_t       _mod;
    };

    struct OnMouseDown
    {
        int                 _x;
        int                 _y;
        models::MouseButton _mouseButton;
        bool                _doubleClick;
    };

    struct OnMouseMove
    {
        int  _absX;
        int  _absY;
        int  _relX;
        int  _relY;
        bool _left;
        bool _middle;
        bool _right;
        bool _x1;
        bool _x2;
    };

    struct OnMouseUp
    {
        int                 _x;
        int                 _y;
        models::MouseButton _mouseButton;
        bool                _doubleClick;
    };

    struct OnMouseWheel
    {
        int          _dx;
        int          _dy;
        uint32_t     _dir; // Set to one of the SDL_MOUSEWHEEL_* defines.
        ::SDL_Keymod _mod;
    };

    struct OnTextInput
    {
        std::string _text; // utf-8
    };
}
