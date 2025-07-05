#pragma once

#include <minire/models/mouse-button.hpp>

namespace minire::events::application
{
    struct OnMouseUp
    {
        int                 _x;
        int                 _y;
        models::MouseButton _mouseButton;
        bool                _doubleClick;

        OnMouseUp(int x,
                  int y,
                  models::MouseButton mouseButton,
                  bool doubleClick)
            : _x(x)
            , _y(y)
            , _mouseButton(mouseButton)
            , _doubleClick(doubleClick)
        {}

    };   
}
