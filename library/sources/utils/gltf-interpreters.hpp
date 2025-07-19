#pragma once

#include <minire/formats/gltf.hpp> // TODO: use forward declaration
#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>
#include <opengl/vertex-buffer.hpp>

#include <limits>
#include <vector>

namespace minire::content { class Manager; }
namespace minire::content { class Lease; }

namespace minire::utils
{
    struct GltfMeshFeatures
    {
        // NOTE: _leases can be released after uploading textures into GPU

        static constexpr size_t kNoIndex = std::numeric_limits<size_t>::max();

        struct Primitive
        {
            models::MeshFeatures _meshFeatures;
            size_t               _materialModel = kNoIndex;
        };

        using Primitives = std::vector<Primitive>;
        using MaterialModels = std::vector<material::Model::Uptr>;
        using Leases = std::vector<std::unique_ptr<content::Lease>>;

        MaterialModels _materialModels;
        Primitives     _primitives;
        Leases         _textureLeases;
    };

    GltfMeshFeatures prefetchGltfFeatures(std::shared_ptr<::tinygltf::Model> const &,
                                         size_t const meshIndex, content::Manager &);

    std::vector<opengl::VertexBuffer>
    createVertexBuffers(::tinygltf::Model const &,
                        size_t const meshIndex,
                        std::vector<material::Program::Locations> const & locationsForPrims);
}
