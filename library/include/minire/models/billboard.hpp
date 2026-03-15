#pragma once

#include <minire/content/id.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

#include <variant>

namespace minire::models
{
    // NOTE: Billboards are not affected by lights.
    // TODO: local 2D-transform (rotate, scale, translate) AND make it lerpable?
    struct Billboard
    {
        struct World
        {
            // in world units
            glm::vec2 _translate;
            glm::vec2 _scale;
        };

        struct Screen
        {
            glm::vec2 _screenOffset;    // pixels
            // TODO: scale
            // TODO: rotate
        };

        using Placement = std::variant<World, Screen>;

        struct Sprite
        {
            content::Id  _texture;
            utils::Patch _source;       // texture area, in pixels
            glm::vec2    _contentSize;  // for resizable content such as NinePatch, in pixels
        };

        struct Label
        {
            text::FormattedString _text;
            content::Id           _fontFace;
        };

        using Content = std::variant<Sprite, Label>;

        Content   _content;
        Placement _placement;
        size_t    _zOrder;
        bool      _visible;
    };
}
