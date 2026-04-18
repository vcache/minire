#pragma once

#include <glm/vec4.hpp>

#include <variant>

namespace minire::models
{
    namespace outline
    {
        struct PixelEdge
        {
            glm::vec4 _color;

            bool operator==(PixelEdge const &) const = default;
        };
    }

    using Outline = std::variant<std::monostate,
                                 outline::PixelEdge>;
}