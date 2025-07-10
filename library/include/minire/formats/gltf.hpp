#pragma once

#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

#include <memory>

namespace minire::formats
{
    using GltfModelSptr = std::shared_ptr<::tinygltf::Model>;

    GltfModelSptr loadGltf(std::string const & filename);
    GltfModelSptr loadGlb(std::string const & filename);
}
