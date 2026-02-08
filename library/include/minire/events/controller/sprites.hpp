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
    struct CreateSprite
    {
        std::string      _id;
        content::Id      _texture;
        utils::Patch     _source;
        glm::vec2        _position;
        glm::vec2        _dimensions;   // it will be used ONLY for a NinePatch
                                        // and will be ignored in all other cases
                                        // TODO: support for a Rect
        utils::MaybeRect _clippingWindow;
        bool             _visible;
        size_t           _zOrder;
    };

    // TODO: tidy up this API (lots of logic duplicated: resize,
    //       move, set area, set clipping window)
    // TODO: make it more consistent w/ Labels API

    // Only applicable for a NinePatch
    // TODO: support for a Rect
    struct ResizeSprite
    {
        std::string      _id;
        glm::vec2        _dimensions;
        utils::MaybeRect _clippingWindow;
    };

    struct MoveSprite
    {
        std::string      _id;
        glm::vec2        _position;
        utils::MaybeRect _clippingWindow;
    };

    struct SetSpriteArea
    {
        std::string      _id;
        glm::vec2        _position;
        glm::vec2        _dimensions;
        utils::MaybeRect _clippingWindow;
    };

    struct SetSpriteClippingWindow
    {
        std::string      _id;
        utils::MaybeRect _clippingWindow;
    };

    struct SetSpriteVisible
    {
        std::string _id;
        bool        _visible;
    };

    struct SetSpriteZOrder
    {
        std::string _id;
        size_t      _zOrder;
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
