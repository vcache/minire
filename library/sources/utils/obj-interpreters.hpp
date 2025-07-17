#pragma once

#include <minire/models/mesh-features.hpp>
#include <opengl/vertex-buffer.hpp>

namespace minire::formats { struct Obj; }

namespace minire::utils
{
    models::MeshFeatures getMeshFeatures(formats::Obj const &);

    opengl::VertexBuffer createVertexBuffer(formats::Obj const &,
                                            int vtxAttribIndex,
                                            int uvAttribIndx,
                                            int normAttrib);
}
