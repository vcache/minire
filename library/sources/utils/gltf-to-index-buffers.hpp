#pragma once

#include <minire/formats/gltf.hpp> // TODO: use forward declaration
#include <opengl/vertex-buffer.hpp>

namespace minire::utils
{
    opengl::VertexBuffer createVertexBuffer(::tinygltf::Model const &,
                                            size_t const meshIndex,
                                            int vtxAttribIndex,
                                            int uvAttribIndx,
                                            int normAttrib,
                                            int tangentAttrib);
}
