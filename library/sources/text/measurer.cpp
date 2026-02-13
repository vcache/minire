#include <text/measurer.hpp>

#include <minire/models/font-face.hpp>
#include <minire/text/formatted-string.hpp>

#include <text/iterator.hpp>

#include <glm/common.hpp>

#include <cassert>

namespace minire::text
{
    glm::vec2 measure(text::FormattedString const & text,
                      models::FontFace const & fontFace)
    {
        glm::vec2 boundary(0);
        iterate(text, glm::vec2(fontFace._glyphWidth, fontFace._glyphHeight),
                [&boundary](text::Symbol const &, glm::vec2 const & position,
                            glm::vec2 const & glyphSize)
                {
                    boundary = glm::max(boundary, position + glyphSize);
                    return true;
                });
        return boundary;
    }

    std::unique_ptr<utils::TextLayout> layout(text::FormattedString const & text,
                                              models::FontFace const & fontFace)
    {
        std::vector<utils::Rect> glyphs;
        glyphs.reserve(text.size());
        iterate(text, glm::vec2(fontFace._glyphWidth, fontFace._glyphHeight),
                [&glyphs](text::Symbol const &, glm::vec2 const & position,
                          glm::vec2 const & glyphSize)
                {
                    assert(glyphSize.x > 0);
                    assert(glyphSize.y > 0);
                    assert(position.x >= 0);
                    assert(position.y >= 0);

                    glm::vec2 const rightBottom = position + glyphSize;
                    glyphs.emplace_back(position.x, position.y,
                                        rightBottom.x, rightBottom.y);
                    return true;
                });
        return std::make_unique<utils::TextLayout>(std::move(glyphs));
    }
}
