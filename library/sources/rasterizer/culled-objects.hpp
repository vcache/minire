#pragma once

#include <rasterizer/cube-shadow-map.hpp>
#include <rasterizer/flat-shadow-map.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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
        bool                _shadowUsePCF;
    };

    // TODO: can be a static array
    using CulledDirectionalLights = std::vector<CulledDirectionalLight>;

    struct CulledPointLight
    {
        glm::vec3            _position;
        glm::vec4           _color;
        glm::vec4           _attenuation;
        CubeShadowMap::Sptr _shadowMap;
        float               _shadowMapFarPlane;
        bool                _shadowUsePCF;
    };

    // TODO: can be a static array
    using CulledPointLights = std::vector<CulledPointLight>;
}