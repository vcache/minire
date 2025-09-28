#pragma once

namespace minire::gui
{
    struct Area
    {
        float _left   = 0;
        float _top    = 0;
        float _width  = 0;
        float _height = 0;

        bool operator==(Area const &) const = default;

        bool contains(float x, float y) const
        {
            return _left <= x && x < _left + _width
                && _top <= y && y < _top + _height;
        }
    };
}
