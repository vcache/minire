#pragma once

#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>

#include <material/types.hpp>

#include <boost/container_hash/hash.hpp> // for hash_combine
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <functional> // For std::hash
#include <memory>
#include <unordered_map>
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

    struct UniquePrimitive
    {
        Mesh const & _mesh; // Mesh = set of (Primitive + Brush)
        size_t       _primitiveIndex;

        bool operator==(UniquePrimitive const & other) const
        {
            return &_mesh == &other._mesh
                && _primitiveIndex == other._primitiveIndex;
        }
    };
}

namespace std
{
    template<>
    struct hash<::minire::rasterizer::UniquePrimitive>
    {
        size_t operator()(::minire::rasterizer::UniquePrimitive const & v) const
        {
            size_t result = 0;
            boost::hash_combine(result, &v._mesh);
            boost::hash_combine(result, v._primitiveIndex);
            return result;
        }
    };
}

namespace minire::rasterizer
{
    struct PrimitiveInstances
    {
        // NOTE: must be properly aligned and padded for VBO
        struct alignas(16) Attribs
        {
            glm::mat4 _transform; // model transform
            glm::vec3 _emissiveFactor;
            uint32_t  _opbId;
        };

        static_assert(0 == (sizeof(Attribs) % 16), "bad size of Attribs");
        static_assert(0 == (offsetof(Attribs, _transform) % 16), "bad offset of _transform");
        static_assert(0 == (offsetof(Attribs, _emissiveFactor) % 16), "bad offset of _emissiveFactor");
        static_assert(offsetof(Attribs, _opbId) == offsetof(Attribs, _emissiveFactor) + sizeof(Attribs::_emissiveFactor),
                      "extra padding detected");
        static_assert(0 == (offsetof(Attribs, _opbId) % 4), "bad offset of _opbId");

        using InstancedAttribs = std::vector<Attribs>;
        using InstancedSkinningVectors = std::vector<material::SkinningVectorSptr>;

        // The sizes of both arrays are guaranteed to be the same
        InstancedAttribs         _attribs;
        InstancedSkinningVectors _skinningVectors;

        size_t size() const
        {
            assert(_attribs.size() == _skinningVectors.size());
            return _attribs.size();
        }
    };

    using CulledPrimitives = std::unordered_map<UniquePrimitive,
                                                PrimitiveInstances>;
}
