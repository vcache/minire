#pragma once

#include <rasterizer/flat-shadow-map.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <vector>

namespace minire::rasterizer
{
    struct CulledDirectionalLight
    {
        glm::vec3           _position; // doesn't affect shading, but required for a shadow pass
        glm::vec3           _direction;
        glm::vec3           _color;
        FlatShadowMap::Sptr _shadowMap;
        glm::mat4           _viewProjection;
    };

    // TODO: can be a static array
    using CulledDirectionalLights = std::vector<CulledDirectionalLight>;
}