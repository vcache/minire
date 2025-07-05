#pragma once

namespace minire::events::application
{
    struct OnMouseMove
    {
        int  _absX;
        int  _absY;
        int  _relX;
        int  _relY;
        bool _left;
        bool _middle;
        bool _right;
        bool _x1;
        bool _x2;

        OnMouseMove(int  absX,
                    int  absY,
                    int  relX,
                    int  relY,
                    bool left,
                    bool middle,
                    bool right,
                    bool x1,
                    bool x2)
            : _absX(absX)
            , _absY(absY)
            , _relX(relX)
            , _relY(relY)
            , _left(left)
            , _middle(middle)
            , _right(right)
            , _x1(x1)
            , _x2(x2)
        {}
    };
}
