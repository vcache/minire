#pragma once

#include <minire/material.hpp> // TODO: included only for Program::Locations

#include <memory>

namespace minire::models { struct VertexBuffer; }
namespace minire::opengl { struct VertexBuffer; }

namespace minire::utils
{
    std::shared_ptr<opengl::VertexBuffer>
    createVertexBuffer(models::VertexBuffer const &,
                       material::Program::Locations const &);
}
