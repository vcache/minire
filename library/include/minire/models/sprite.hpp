#pragma once

#include <minire/content/id.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

#include <optional>

namespace minire::models
{
    namespace sprite
    {
        struct Image
        {
            content::Id  _texture; // TODO: or direct texture pointer
            utils::Patch _patch; // in pixels, on a texture

            Image(content::Id texture,
                  utils::Patch patch = std::monostate())
                : _texture(std::move(texture))
                , _patch(std::move(patch))
            {}

            bool operator==(Image const &) const = default;
        };

        using MaybeImage = std::optional<Image>;
    }

    struct Sprite
    {
        sprite::Image    _image;
        glm::vec2        _position;
        glm::vec2        _dimensions; // is only used for NinePatch
        utils::MaybeRect _clippingWindow;
        size_t           _zOrder;
        bool             _visible;
    };
}
