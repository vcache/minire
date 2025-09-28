#pragma once

#include <glm/vec2.hpp>

#include <optional>

namespace minire::utils
{
    // TODO: move to glm::vec4
    struct Rect
    {
        float _left;
        float _top;
        float _right;
        float _bottom;

        explicit Rect(float left = .0f,
                      float top = .0f,
                      float right = .0f,
                      float bottom = .0f)
            : _left(left)
            , _top(top)
            , _right(right)
            , _bottom(bottom)
        {}

        Rect & operator+=(float offset)
        {
            _left += offset;
            _top += offset;
            _right += offset;
            _bottom += offset;
            return *this;
        }
    };

    using MaybeRect = std::optional<Rect>;

    /*
            outX0  inX0           inX1  outX1
                +  +                 +  +
                |  |                 |  |
                |  |                 |  |
             +--+--+-----------------+--+--+
             |                             |
             | ++  +-----------------+  ++ |
        +----+ +                         + +----+outY0
             |                             |
        +----+ +                         + +----+inY0
             | |                         | |
             | |                         | |
             | |                         | |
             | |                         | |
        +----+ +                         + +----+inY1
             |                             |
        +----+ +                         + +----+outY1
             | ++  +-----------------+  ++ |
             |                             |
             +--+--+-----------------+--+--+
                |  |     boundary    |  |
                |  |                 |  |
                +  +                 +  +
    */
    struct NinePatch
    {
        Rect _boundary; // outter boundary of a 9-patch
        Rect _out;
        Rect _in;
    };
}
