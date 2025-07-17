#pragma once

#include <minire/content/id.hpp>

#include <glm/vec3.hpp>

#include <variant>

namespace minire::models
{
    using TextureMap = std::variant<std::monostate,
                                    float,
                                    glm::vec3,
                                    content::Id>;
}