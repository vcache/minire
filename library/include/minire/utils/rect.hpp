#pragma once

#include <glm/vec2.hpp>

#include <optional>
#include <variant>

namespace minire::utils
{
    // TODO: move to glm::vec4
    struct Rect
    {
        float _left;
        float _top;
        float _right;
        float _bottom;

        explicit Rect(float left,
                      float top,
                      float right,
                      float bottom)
            : _left(left)
            , _top(top)
            , _right(right)
            , _bottom(bottom)
        {}

        explicit Rect(float v = 0.0f)
            : _left(v)
            , _top(v)
            , _right(v)
            , _bottom(v)
        {}

        Rect & operator+=(float offset)
        {
            _left += offset;
            _top += offset;
            _right += offset;
            _bottom += offset;
            return *this;
        }

        bool operator==(Rect const &) const = default;
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

        bool operator==(NinePatch const &) const = default;
    };

    glm::vec2 defaultSize(NinePatch const &);

    using Patch = std::variant<std::monostate,      // whole texture
                               utils::Rect,         // specific part of texture
                               utils::NinePatch>;   // 9-patch from texture
}
