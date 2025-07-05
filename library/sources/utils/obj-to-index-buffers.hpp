#pragma once

#include <opengl/index-buffers.hpp>

namespace minire::formats { struct Obj; }

namespace minire::utils
{
    opengl::IndexBuffers createIndexBuffers(formats::Obj const &,
                                            int vtxAttribIndex,
                                            int uvAttribIndx,
                                            int normAttrib);
}
