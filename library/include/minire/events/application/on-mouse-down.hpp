#pragma once

#include <minire/models/mouse-button.hpp>

namespace minire::events::application
{
    struct OnMouseDown
    {
        int                 _x;
        int                 _y;
        models::MouseButton _mouseButton;
        bool                _doubleClick;

        OnMouseDown(int x,
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
