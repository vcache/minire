#include <text/measurer.hpp>

#include <minire/models/font-face.hpp>
#include <minire/text/formatted-string.hpp>

#include <text/iterator.hpp>

#include <glm/common.hpp>

namespace minire::text
{
    glm::vec2 measure(text::FormattedString const & text,
                      models::FontFace const & fontFace)
    {
        glm::vec2 boundary(0);
        iterate(text, glm::vec2(fontFace._glyphWidth, fontFace._glyphHeight),
                [&boundary](text::TextFormat const &, wchar_t,
                            glm::vec2 const & position, glm::vec2 const & glyphSize)
                {
                    boundary = glm::max(boundary, position + glyphSize);
                    return true;
                });
        return boundary;
    }
}
