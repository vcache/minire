#pragma once

#include <SDL2/SDL_mouse.h>

namespace minire::events::controller
{
    struct SetSystemCursor
    {
        ::SDL_SystemCursor _systemCursor;
    };
}
