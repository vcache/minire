#pragma once

#include <opengl/vertex-buffer.hpp>

namespace minire::formats { struct Obj; }

namespace minire::utils
{
    opengl::VertexBuffer createVertexBuffer(formats::Obj const &,
                                            int vtxAttribIndex,
                                            int uvAttribIndx,
                                            int normAttrib);
}
