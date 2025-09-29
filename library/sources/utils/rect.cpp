#include <minire/utils/rect.hpp>

#include <cassert>

namespace minire::utils
{
    glm::vec2 defaultSize(NinePatch const & np)
    {
        assert(np._in._right >= np._in._left);
        assert(np._out._left >= np._boundary._left);
        assert(np._boundary._right >= np._out._right);

        assert(np._in._bottom >= np._in._top);
        assert(np._out._top >= np._boundary._top);
        assert(np._boundary._bottom >= np._out._bottom);

        auto width = (np._in._right - np._in._left + 1) +
                     (np._out._left - np._boundary._left + 1) +
                     (np._boundary._right - np._out._right + 1);

        auto height = (np._in._bottom - np._in._top + 1) +
                      (np._out._top - np._boundary._top + 1) +
                      (np._boundary._bottom - np._out._bottom + 1);

        return glm::vec2(width, height);
    }
}
