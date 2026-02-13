#pragma once

#include <glm/vec2.hpp>

#include <minire/text/formatted-string.hpp>
#include <minire/text/unicode.hpp>

namespace minire::text
{
    // TODO: unicode has a bunch of crappy line terminators:
    //       https://en.wikipedia.org/wiki/Newline#Unicode
    template<typename Function>
    void iterate(text::FormattedString const & text,
                 glm::vec2 const & glyphSize,
                 Function function)
    {
        glm::vec2 position(0);
        for(Symbol const & symbol : text)
        {
            if (!isLineBreak(symbol.codePoint()))
            {
                if (!function(symbol, position, glyphSize))
                    break;
                position.x += glyphSize.x;
            }
            else
            {
                position.x = 0;
                position.y += glyphSize.y;
            }
        }
    }
}
