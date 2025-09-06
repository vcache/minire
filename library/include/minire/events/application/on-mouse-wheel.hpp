#pragma once

#include <minire/events/application/base.hpp>

namespace minire::events::application
{
    struct OnMouseWheel : public Base
    {
        static models::QueryEventFilter constexpr kQueueEventFilter = models::QueryEventFilter::kOnMouseWheel;

        int _dx;
        int _dy;

        template<typename... BaseArgs>
        OnMouseWheel(int dx, int dy,
                     BaseArgs && ... baseArgs)
            : Base(std::forward<BaseArgs>(baseArgs)...)
            , _dx(dx)
            , _dy(dy)
        {}
    };   
}
