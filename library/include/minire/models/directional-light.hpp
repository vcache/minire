#pragma once

#include <glm/vec4.hpp>

namespace minire::models
{
    struct DirectionalLight
    {
        glm::vec3 _color;

        DirectionalLight(glm::vec3 const & color)
            : _color(color)
        {}

         void lerp(DirectionalLight const & prev,
                   DirectionalLight const & last,
                   float const weight)
        {
            _color = glm::mix(prev._color, last._color, weight);
        }
    };
}