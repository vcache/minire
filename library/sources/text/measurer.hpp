#pragma once

#include <glm/vec2.hpp>

namespace minire::models { struct FontFace; }

namespace minire::text
{
    class FormattedString;

    glm::vec2 measure(text::FormattedString const &,
                      models::FontFace const &);
}
