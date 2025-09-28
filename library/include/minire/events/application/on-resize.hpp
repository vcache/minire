#pragma once

#include <minire/events/application/base.hpp>

#include <cstddef>

namespace minire::events::application
{
    struct OnResize : public Base
    {
        size_t _width;
        size_t _height;

        template<typename... BaseArgs>
        OnResize(size_t width, size_t height,
                 BaseArgs && ... baseArgs)
            : Base(std::forward<BaseArgs>(baseArgs)...)
            , _width(width)
            , _height(height)
        {}
    };
}
