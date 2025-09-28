#pragma once

#include <minire/text/formatted-string.hpp>
#include <minire/text/symbol.hpp>
#include <minire/text/text-format.hpp>

#include <glm/vec2.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace minire::events::controller
{
    struct CreateLabel
    {
        std::string           _id;
        text::FormattedString _text;
        content::Id           _fontFace;
        glm::vec2             _position;
        bool                  _visible;
        size_t                _zOrder;
    };

    struct MoveLabel
    {
        std::string _id;
        glm::vec2   _position;
    };

    struct SetLabelVisible
    {
        std::string _id;
        bool        _visible;
    };

    struct SetLabelText
    {
        std::string           _id;
        text::FormattedString _text;
    };

    struct SetLabelFontFace
    {
        std::string _id;
        content::Id _fontFace;
    };

    struct SetLabelClipping
    {
        std::string              _id;
        std::optional<glm::vec2> _maxSize;
    };

    struct RemoveLabel
    {
        std::string _id;
    };

    struct BulkSetLabelZOrders
    {
        using Item = std::pair<std::string, size_t>;
        std::vector<Item> _items;
    };
}
