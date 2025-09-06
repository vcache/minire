#pragma once

#include <minire/events/application/base.hpp>

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_scancode.h>

#include <cstddef>

namespace minire::events::application
{
    struct OnKeyDown : public Base
    {
        static models::QueryEventFilter constexpr kQueueEventFilter = models::QueryEventFilter::kOnKeyDown;

        ::SDL_Keycode  _key;
        ::SDL_Scancode _code;
        uint16_t       _mod;

        template<typename... BaseArgs>
        OnKeyDown(::SDL_Keycode  key,
                  ::SDL_Scancode code,
                  uint16_t       mod,
                  BaseArgs && ... baseArgs)
            : Base(std::forward<BaseArgs>(baseArgs)...)
            , _key(key)
            , _code(code)
            , _mod(mod)
        {}
    };
}
