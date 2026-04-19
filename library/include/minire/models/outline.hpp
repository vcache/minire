#pragma once

#include <glm/vec4.hpp>

#include <variant>

namespace minire::models
{
    namespace outline
    {
        struct NoOutline
        {
            bool operator==(NoOutline const &) const = default;
        };

        struct PixelEdge
        {
            glm::vec4 _color;

            bool operator==(PixelEdge const &) const = default;
        };
    }

    using Outline = std::variant<std::monostate,        // automatic outline (inherited from parent)
                                 outline::NoOutline,    // explicit absence of an outline
                                 outline::PixelEdge>;
}