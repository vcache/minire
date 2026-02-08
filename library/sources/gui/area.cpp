#include <minire/gui/area.hpp>

#include <cassert>
#include <cmath>

namespace minire::gui
{
    Area intersection(Area const & lhs, Area const & rhs)
    {
        float const left = std::max(lhs._left, rhs._left);
        float const top = std::max(lhs._top, rhs._top);
        float const right = std::min(lhs._left + lhs._width,
                                     rhs._left + rhs._width);
        float const bottom = std::min(lhs._top + lhs._height,
                                      rhs._top + rhs._height);

        return Area
        {
            ._left   = left,
            ._top    = top,
            ._width  = right >= left ? right - left : 0,
            ._height = bottom >= top ? bottom - top : 0,
        };
    }
}
