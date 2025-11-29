#pragma once

#include <glm/vec4.hpp>

namespace minire::models
{
    struct DirectionalLight
    {
        glm::vec3 _color;
        bool      _enableShadows;

        DirectionalLight(glm::vec3 const & color,
                         bool const enableShadows = false)
            : _color(color)
            , _enableShadows(enableShadows)
        {}

         void lerp(DirectionalLight const & prev,
                   DirectionalLight const & last,
                   float const weight)
        {
            _color = glm::mix(prev._color, last._color, weight);
        }
    };
}