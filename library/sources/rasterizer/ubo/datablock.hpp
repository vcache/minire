#pragma once

#include <minire/utils/always-false.hpp>
#include <minire/utils/demangle.hpp>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <cstddef>  // for offsetof
#include <string>

namespace minire::rasterizer::ubo
{
    /**
     * Layout rules:
     *
     *  N = 4
     *
     *  - Scalar => N (int, float, bool)
     *  - Vector => 2N (vec2) or 4N (vec3, vec4)
     *  - Array => Each element has a base alignment equal to that of a vec4.
     *  - Matrices => Stored as a large array of column vectors,
     *                where each of those vectors has a base alignment of vec4.
     *  - Struct => Equal to the computed size of its elements according to the previous rules,
     *              but padded to a multiple of the size of a vec4.
     *
     *  Spec: https://www.opengl.org/registry/specs/ARB/uniform_buffer_object.txt
     *  See also: https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
     * */

    static constexpr size_t kStd140N = 4;

    struct DirectionalLight
    {
        // world space
        alignas(4 * kStd140N) glm::vec3 _direction = glm::vec3(0);

        // linear values, for higher intesity should be >1.0
        alignas(4 * kStd140N) glm::vec3 _color = glm::vec3(0);

        // light-space transform matrix
        alignas(4 * kStd140N) glm::mat4 _viewProjection = glm::mat4(1.0f);

        // which method to use:
        // 0 - no shadows,  1 - std; 2 - std w/ pcf; 3 - ESM; 4 - LogESM
        alignas(1 * kStd140N) uint32_t  _method = 0;

        // optional argument of a method
        alignas(1 * kStd140N) float     _methodArg = 0;

        // which bias to be used for normals:
        // 0 - constant (_normalBiasBase only)
        // 1 - slope-scaled (_normalBiasBase + _normalBiasMax)
        alignas(1 * kStd140N) uint32_t  _normalBiasMode = 0;
        alignas(1 * kStd140N) float     _normalBiasBase = 0;
        alignas(1 * kStd140N) float     _normalBiasMax = 0;

        // which bias to be used for depth:
        // 0 - constant (_depthBiasBase only)
        // 1 - slope-scaled (_depthBiasBase + _depthBiasMax)
        alignas(1 * kStd140N) uint32_t  _depthBiasMode = 0;
        alignas(1 * kStd140N) float     _depthBiasBase = 0;
        alignas(1 * kStd140N) float     _depthBiasMax = 0;

        // edges for a "smoothstep" of a shadow (to prevent light bleeding)
        alignas(1 * kStd140N) float     _smoothStepLeft = 0;
        alignas(1 * kStd140N) float     _smoothStepRight = 0;
    };

    struct PointLight
    {
        // w = 1.0
        alignas(4 * kStd140N) glm::vec4 _position = glm::vec4(0);

        // alpha is an intensity
        alignas(4 * kStd140N) glm::vec4 _color = glm::vec4(0);

        // x - constant, y - linear, z - quadratic
        alignas(4 * kStd140N) glm::vec4 _attenuation = glm::vec4(0);

        // far plane used in a projection matrix of a light map
        alignas(1 * kStd140N) float     _shadowMapFarPlane = 0;

        // which method to use:
        // 0 - no shadows,  1 - std; 2 - std w/ pcf; 3 - ESM; 4 - LogESM
        alignas(1 * kStd140N) uint32_t  _method = 0;

        // optional argument of a method
        alignas(1 * kStd140N) float     _methodArg = 0;

        // which bias to be used for normals:
        // 0 - constant (_normalBiasBase only)
        // 1 - slope-scaled (_normalBiasBase + _normalBiasMax)
        alignas(1 * kStd140N) uint32_t  _normalBiasMode = 0;
        alignas(1 * kStd140N) float     _normalBiasBase = 0;
        alignas(1 * kStd140N) float     _normalBiasMax = 0;

        // which bias to be used for depth:
        // 0 - constant (_depthBiasBase only)
        // 1 - slope-scaled (_depthBiasBase + _depthBiasMax)
        alignas(1 * kStd140N) uint32_t  _depthBiasMode = 0;
        alignas(1 * kStd140N) float     _depthBiasBase = 0;
        alignas(1 * kStd140N) float     _depthBiasMax = 0;

        // edges for a "smoothstep" of a shadow (to prevent light bleeding)
        alignas(1 * kStd140N) float     _smoothStepLeft = 0;
        alignas(1 * kStd140N) float     _smoothStepRight = 0;
    };

    struct Datablock
    {
        static constexpr uint32_t kMaxPointLights = 16;
        static constexpr uint32_t kMaxDirectionalLights = 4;

        alignas(4 * kStd140N) glm::mat4        _viewProjection = glm::mat4(1.0f);
        alignas(4 * kStd140N) glm::vec4        _viewPosition = glm::vec4(0);
        alignas(4 * kStd140N) DirectionalLight _directionalLights[kMaxDirectionalLights];
        alignas(1 * kStd140N) uint32_t         _directionalLightsCount = 0;
        alignas(4 * kStd140N) PointLight       _pointLights[kMaxPointLights];
        alignas(1 * kStd140N) uint32_t         _pointLightsCount = 0;
    };

    // Interface builder //

    template<typename T>
    [[noreturn]] inline std::string makeInterfaceBlock()
    {
        static_assert(utils::kAlwaysFalse<T>::value,
                      "no specialization exists");
    }

    template<>
    inline std::string makeInterfaceBlock<DirectionalLight>()
    {
        return R"(
        struct BznkDirectionalLight
        {
            vec3  _direction;
            vec3  _color;
            mat4  _viewProjection;
            uint  _method;
            float _methodArg;
            uint  _normalBiasMode;
            float _normalBiasBase;
            float _normalBiasMax;
            uint  _depthBiasMode;
            float _depthBiasBase;
            float _depthBiasMax;
            float _smoothStepLeft;
            float _smoothStepRight;
        };
        )";
    }

    template<>
    inline std::string makeInterfaceBlock<PointLight>()
    {
        return R"(
        struct BznkPointLight
        {
            vec4  _position;
            vec4  _color;
            vec4  _attenuation;
            float _shadowMapFarPlane;
            uint  _method;
            float _methodArg;
            uint  _normalBiasMode;
            float _normalBiasBase;
            float _normalBiasMax;
            uint  _depthBiasMode;
            float _depthBiasBase;
            float _depthBiasMax;
            float _smoothStepLeft;
            float _smoothStepRight;
        };
        )";
    }

    template<>
    inline std::string makeInterfaceBlock<Datablock>()
    {
        static std::string const kMaxPointLights = std::to_string(Datablock::kMaxPointLights);
        static std::string const kMaxDirectionalLights = std::to_string(Datablock::kMaxDirectionalLights);

        return makeInterfaceBlock<DirectionalLight>() + "\n" +
               makeInterfaceBlock<PointLight>() + "\n" +
        R"(
        layout(std140) uniform BznkDatablock
        {
            mat4                 _viewProjection;
            vec4                 _viewPosition;
            BznkDirectionalLight _directionalLights[)" + kMaxDirectionalLights + R"(];
            uint                 _directionalLightsCount;
            BznkPointLight       _pointLights[)" + kMaxPointLights + R"(];
            uint                 _pointLightsCount;
        };
        )";
    }
}
