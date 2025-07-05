#pragma once

namespace minire::events::application
{
    struct OnMouseWheel
    {
        int _dx;
        int _dy;

        OnMouseWheel(int dx, int dy)
            : _dx(dx)
            , _dy(dy)
        {}
    };   
}
