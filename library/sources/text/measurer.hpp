#pragma once

#include <minire/utils/glyph-grid.hpp>

#include <glm/vec2.hpp>

#include <limits>
#include <memory>

namespace minire::models { struct FontFace; }

namespace minire::text
{
    class FormattedString;

    glm::vec2 measure(text::FormattedString const &,
                      models::FontFace const &);

    // TODO: it will not work correctly for multiline strings,
    //       because '\n' won't be included into TextLayout::_heap.
    //       Thereby, it will be shorter than input text (layoutOf may crash!).
    std::unique_ptr<utils::TextLayout> layout(text::FormattedString const &,
                                              models::FontFace const &);
}
