#include <minire/models/mouse-button.hpp>

#include <minire/errors.hpp>

#include <SDL2/SDL_mouse.h>

namespace minire::models
{
    MouseButton mouseButtonFromSdl(size_t sdlButton)
    {
        switch(sdlButton)
        {
            case SDL_BUTTON_LEFT: return MouseButton::kLeft;
            case SDL_BUTTON_MIDDLE: return MouseButton::kMiddle;
            case SDL_BUTTON_RIGHT: return MouseButton::kRight;
            case SDL_BUTTON_X1: return MouseButton::kX1;
            case SDL_BUTTON_X2: return MouseButton::kX2;
        }

        MINIRE_THROW("unknown sdl button: {:#x}", sdlButton);
    }
}
