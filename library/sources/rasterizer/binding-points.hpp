#pragma once

namespace minire::rasterizer
{
    // UBOs
    static constexpr uint32_t kModelsBindingPoint        = 0;
    static constexpr uint32_t kScreenPassBindingPoint    = 1;
    static constexpr uint32_t kClusterConfigBindingPoint = 2;

    // SSBOs
    static constexpr uint32_t kSkinningVectrorBindingPoint     = 0;
    static constexpr uint32_t kClusterPointLightsBindingPoint  = 1;
    static constexpr uint32_t kClusterGridBindingPoint         = 2;
    static constexpr uint32_t kClusterLightIndicesBindingPoint = 3;
}
