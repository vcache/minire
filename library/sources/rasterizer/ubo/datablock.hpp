#pragma once

#include <minire/errors.hpp>
#include <minire/utils/demangle.hpp>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <cstddef>  // for offsetof
#include <string>
#include <typeinfo> // for typeid

namespace minire::rasterizer::ubo
{
    /*!
     * To pack data as tight as possible color vectors
     * also storing attenuation factors.
     * */
    struct PointLight
    {
        static constexpr uint32_t kN = 4;

        // w = 1.0
        alignas(4 * kN) glm::vec4 _position = glm::vec4(0);

        // alpha not used
        alignas(4 * kN) glm::vec4 _color = glm::vec4(0);

        // x - constant, y - linear, z - quadratic
        alignas(4 * kN) glm::vec4 _attenuation = glm::vec4(0);
    };

    struct Datablock
    {
        static constexpr uint32_t kMaxLights = 16;
        static constexpr uint32_t kN = 4;

        alignas(4 * kN) glm::mat4  _viewProjection = glm::mat4(1.0f);
        alignas(4 * kN) PointLight _pointLights[kMaxLights];
        alignas(4 * kN) glm::vec4  _viewPosition = glm::vec4(0);
        alignas(kN)     uint32_t   _lightsCount = 0;
    };

    // Interface builder //

    template<typename T>
    inline std::string makeInterfaceBlock()
    {
        MINIRE_THROW("no makeInterfaceBlock for " <<
                     utils::demangle<T>());
    }

    template<>
    inline std::string makeInterfaceBlock<PointLight>()
    {
        return R"(
        struct BznkPointLight
        {
            vec4 _position;
            vec4 _color;
            vec4 _attenuation;
        };
        )";
    }

    template<>
    inline std::string makeInterfaceBlock<Datablock>()
    {
        const std::string kMaxLights = std::to_string(Datablock::kMaxLights);
        return makeInterfaceBlock<PointLight>() +
        R"(
        layout(std140) uniform BznkDatablock
        {
            mat4           _viewProjection;
            BznkPointLight _pointLights[)" + kMaxLights + R"(];
            vec4           _viewPosition;
            uint           _lightsCount;
        };
        )";
    }
}
