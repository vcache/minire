#pragma once

#include <cstddef>

namespace minire::models
{
    enum class MouseButton
    {
        kLeft,
        kMiddle,
        kRight,
        kX1,
        kX2,
    };

    MouseButton mouseButtonFromSdl(size_t sdlButton);
}
