#pragma once

#include <minire/content/id.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace minire::events::controller
{

    // TODO: maybe use size_t as ID instead string

    struct CreateSprite
    {
        std::string _id;
        content::Id _texture;
        utils::Rect _tile; // TODO: make it optional
        glm::vec2   _position;
        bool        _visible;
        size_t      _z;
    };

    struct CreateNinePatch
    {
        std::string      _id;
        content::Id      _texture;
        utils::NinePatch _tile;
        glm::vec2        _position;
        glm::vec2        _dimensions;
        bool             _visible;
        size_t           _z;
    };

    struct ResizeNinePatch
    {
        std::string _id;
        glm::vec2   _dimensions;
    };

    // TODO: create _delta version
    struct MoveSprite
    {
        std::string _id;
        glm::vec2   _position;
    };

    struct VisibleSprite
    {
        std::string _id;
        bool        _visible;
    };

    struct RemoveSprite
    {
        std::string _id;
    };

    struct BulkSetSpriteZOrders
    {
        using Item = std::pair<std::string, size_t>;
        std::vector<Item> _items;
    };
}
