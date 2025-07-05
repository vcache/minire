#pragma once

#include <minire/text/formatted-string.hpp>
#include <minire/text/symbol.hpp>
#include <minire/text/text-format.hpp>

#include <glm/vec2.hpp>

#include <string>
#include <vector>
#include <utility>

namespace minire::events::controller
{
    struct CreateLabel
    {
        std::string _id;
        size_t      _z;
        bool        _visible;
    };

    struct ResizeLabel
    {
        std::string _id;
        size_t      _rows;
        size_t      _cols;
    };

    struct MoveLabel
    {
        std::string _id;
        float       _x;
        float       _y;
    };

    struct SetCharLabel
    {
        std::string      _id;
        text::TextFormat _format;
        wchar_t          _char;
        size_t           _row;
        size_t           _col;
    };

    struct SetSymbolLabel
    {
        std::string  _id;
        size_t       _row;
        size_t       _col;
        text::Symbol _symbol;
    };

    struct UnsetCharLabel
    {
        std::string _id;
        size_t      _row;
        size_t      _col;
    };

    struct SetStringLabel
    {
        std::string           _id;
        text::FormattedString _string;
        size_t                _row;
        size_t                _col;
    };

    struct SetLabelCursor
    {
        std::string _id;
        size_t      _row;
        size_t      _col;
    };

    struct UnsetLabelCursor
    {
        std::string _id;
    };

    struct SetLabelVisible
    {
        std::string _id;
        bool        _visible;
    };

    struct SetLabelFonts
    {
        std::string _id;
        std::string _fontName; // TODO: fontName -> fontId
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
