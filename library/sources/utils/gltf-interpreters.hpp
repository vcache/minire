#pragma once

#include <minire/formats/gltf.hpp> // TODO: use forward declaration
#include <minire/models/mesh-features.hpp>
#include <opengl/vertex-buffer.hpp>

namespace minire::utils
{
    models::MeshFeatures getMeshFeatures(::tinygltf::Model const &,
                                         size_t const meshIndex);

    opengl::VertexBuffer createVertexBuffer(::tinygltf::Model const &,
                                            size_t const meshIndex,
                                            int vtxAttribIndex,
                                            int uvAttribIndx,
                                            int normAttrib,
                                            int tangentAttrib);
}
