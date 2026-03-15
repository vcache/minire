#pragma once

#include <minire/content/id.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

namespace minire::models
{
    struct Label
    {
        text::FormattedString _text;
        content::Id           _fontFace;    // TODO: allow loading from models::FontFace
        glm::vec2             _position;
        utils::MaybeRect      _clippingWindow;
        size_t                _zOrder;
        bool                  _visible;
    };
}
