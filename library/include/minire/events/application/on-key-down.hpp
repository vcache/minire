#pragma once

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>

#include <cstddef>

namespace minire::events::application
{
    struct OnKeyDown
    {
        ::SDL_Keycode  _key;
        ::SDL_Scancode _code;
        uint16_t       _mod;

        OnKeyDown(::SDL_Keycode  key,
                  ::SDL_Scancode code,
                  uint16_t       mod)
            : _key(key)
            , _code(code)
            , _mod(mod)
        {}
    };
}
