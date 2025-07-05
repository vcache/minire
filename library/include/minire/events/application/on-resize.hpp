#pragma once

#include <cstddef>

namespace minire::events::application
{
    struct OnResize
    {
        size_t _width;
        size_t _height;

        OnResize(size_t width, size_t height)
            : _width(width)
            , _height(height)
        {}
    };   
}
