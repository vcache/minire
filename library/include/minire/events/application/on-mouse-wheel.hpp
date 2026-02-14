#pragma once

#include <minire/events/application/base.hpp>

#include <SDL_keycode.h>

namespace minire::events::application
{
    struct OnMouseWheel : public Base
    {
        int          _dx;
        int          _dy;
        uint32_t     _dir; // Set to one of the SDL_MOUSEWHEEL_* defines.
        ::SDL_Keymod _mod;

        template<typename... BaseArgs>
        OnMouseWheel(int dx, int dy,
                     uint32_t dir,
                     ::SDL_Keymod mod,
                     BaseArgs && ... baseArgs)
            : Base(std::forward<BaseArgs>(baseArgs)...)
            , _dx(dx)
            , _dy(dy)
            , _dir(dir)
            , _mod(mod)
        {}
    };
}
