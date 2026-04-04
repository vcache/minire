#pragma once

#include <minire/models/shadow-params.hpp>

#include <glm/vec4.hpp>

namespace minire::models
{
    struct DirectionalLight
    {
        glm::vec3         _color;
        MaybeShadowParams _shadowParams;
        bool              _visible;

        void lerp(DirectionalLight const & prev,
                  DirectionalLight const & last,
                  float const weight)
        {
            _color = glm::mix(prev._color, last._color, weight);
        }
    };
}
