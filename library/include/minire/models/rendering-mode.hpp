#pragma once

#include <cstdint>
#include <variant>

namespace minire::models
{
    namespace rendering_mode
    {
        struct Forward
        {
            bool operator==(Forward const &) const = default;
        };

        struct ClusteredForward
        {
            uint32_t _clustersX = 16;
            uint32_t _clustersY = 9;
            uint32_t _clustersZ = 24;
            uint32_t _maxLightsPerCluster = 100;
            uint32_t _maxPointLights = 1024;
            uint32_t _maxShadowLights = 16;

            bool operator==(ClusteredForward const &) const = default;
        };
    }

    using RenderingMode = std::variant<rendering_mode::Forward,
                                       rendering_mode::ClusteredForward>;
}
