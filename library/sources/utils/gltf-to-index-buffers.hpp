#pragma once

#include <minire/formats/gltf.hpp> // TODO: use forward declaration
#include <opengl/index-buffers.hpp>

namespace minire::utils
{
    opengl::IndexBuffers createIndexBuffers(::tinygltf::Model const &,
                                            size_t const meshIndex,
                                            int vtxAttribIndex,
                                            int uvAttribIndx,
                                            int normAttrib,
                                            int tangentAttrib);
}
