#pragma once

#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <vector>

namespace minire::rasterizer
{
    class FlatShadowMap;
    using FlatShadowMapSptr = std::shared_ptr<FlatShadowMap>;

    struct CulledDirectionalLight
    {
        glm::vec3         _position; // doesn't affect shading, but required for a shadow pass
        glm::vec3         _direction;
        glm::vec3         _color;
        FlatShadowMapSptr _shadowMap;
        glm::mat4         _viewProjection;
    };

    // TODO: can be a static array
    using CulledDirectionalLights = std::vector<CulledDirectionalLight>;

    class CubeShadowMap;
    using CubeShadowMapSptr = std::shared_ptr<CubeShadowMap>;

    struct CulledPointLight
    {
        glm::vec3         _position;
        glm::vec4         _color;
        glm::vec4         _attenuation;
        CubeShadowMapSptr _shadowMap;
        float             _shadowMapFarPlane;
    };

    // TODO: can be a static array
    using CulledPointLights = std::vector<CulledPointLight>;

    class Mesh;

    struct CulledPrimitive
    {
        Mesh const &             _mesh;
        size_t                   _primitiveIndex;
        glm::vec3                _emissiveFactor;
        glm::mat4                _transform;
        material::SkinningVector _skinningVector;
        size_t const             _obpId;
    };

    using CulledPrimitives = std::vector<CulledPrimitive>;
}
