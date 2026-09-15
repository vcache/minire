#pragma once

#include <cstddef> // for size_t

namespace minire::models
{
    struct DisplayMode
    {
        size_t _width;
        size_t _height;
        size_t _refreshRate;

        bool operator==(DisplayMode const &) const = default;
    };
}