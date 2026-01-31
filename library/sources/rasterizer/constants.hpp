#pragma once

#include <string>

namespace minire::rasterizer
{
    struct Constants
    {
        static constexpr size_t kMaxBones = 128;
        static std::string kPbrKit;
        static std::string kModelSkinningKit;
        static std::string kPbrVertShader;
        static std::string kPbrFragShader;
    };
}
