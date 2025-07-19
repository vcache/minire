#pragma once

#include <minire/formats/gltf.hpp> // TODO: use forward declaration
#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>
#include <opengl/vertex-buffer.hpp>

#include <vector>

namespace minire::content { class Manager; }
namespace minire::content { class Lease; }

namespace minire::utils
{
    models::MeshFeatures getMeshFeatures(::tinygltf::Model const &,
                                         size_t const meshIndex);

    struct GltfMaterial
    {
        using Leases = std::vector<std::unique_ptr<content::Lease>>;

        // NOTE: leases can be released after uploading textures into GPU

        material::Model::Uptr _materialModel;
        Leases                _textureLeases;
    };

    GltfMaterial createMaterialModel(std::shared_ptr<::tinygltf::Model> const &,
                                     size_t const meshIndex,
                                     content::Manager &);

    opengl::VertexBuffer createVertexBuffer(::tinygltf::Model const &,
                                            size_t const meshIndex,
                                            int vtxAttribIndex,
                                            int uvAttribIndx,
                                            int normAttrib,
                                            int tangentAttrib);
}
