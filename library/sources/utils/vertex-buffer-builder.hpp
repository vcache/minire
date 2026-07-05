#pragma once

#include <memory>

namespace minire::models { struct VertexBuffer; }
namespace minire::opengl { struct VertexBuffer; }
namespace minire::material { struct Locations; }

namespace minire::utils
{
    std::shared_ptr<opengl::VertexBuffer>
    createVertexBuffer(models::VertexBuffer const &,
                       material::Locations const &);
}
