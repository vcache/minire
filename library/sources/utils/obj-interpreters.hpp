#pragma once

#include <minire/models/mesh-features.hpp>
#include <opengl/shared-vertex-buffer.hpp>

#include <memory>

namespace minire::formats { struct Obj; }
namespace minire::material { class Locations; }

namespace minire::utils
{
    models::MeshFeatures getMeshFeatures(formats::Obj const &);

    std::unique_ptr<opengl::SharedVertexSubBuffer>
    createVertexBuffer(formats::Obj const &,
                       material::Locations const &,
                       opengl::SharedVertexBuffer &);
}
