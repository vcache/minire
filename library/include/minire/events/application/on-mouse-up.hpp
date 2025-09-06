#pragma once

#include <minire/events/application/base.hpp>
#include <minire/models/mouse-button.hpp>

namespace minire::events::application
{
    struct OnMouseUp : public Base
    {
        int                 _x;
        int                 _y;
        models::MouseButton _mouseButton;
        bool                _doubleClick;

        template<typename... BaseArgs>
        OnMouseUp(int x,
                  int y,
                  models::MouseButton mouseButton,
                  bool doubleClick,
                  BaseArgs && ... baseArgs)
            : Base(std::forward<BaseArgs>(baseArgs)...)
            , _x(x)
            , _y(y)
            , _mouseButton(mouseButton)
            , _doubleClick(doubleClick)
        {}

    };   
}
