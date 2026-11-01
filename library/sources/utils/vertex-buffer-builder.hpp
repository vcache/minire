#pragma once

#include <glad/gl.h>
#include <minire/models/vertex-buffer.hpp>

#include <memory>

namespace minire::opengl { struct VertexBuffer; }
namespace minire::opengl { class SharedVertexBuffer; }
namespace minire::opengl { class SharedVertexSubBuffer; }
namespace minire::material { struct Locations; }

namespace minire::utils
{
    GLenum convertDrawMode(models::VertexBuffer::DrawMode drawMode);

    std::shared_ptr<opengl::VertexBuffer>
    createVertexBuffer(models::VertexBuffer const &,
                       material::Locations const &);

    std::unique_ptr<opengl::SharedVertexSubBuffer>
    createVertexSubBuffer(models::VertexBuffer const & vertexBuffer,
                          opengl::SharedVertexBuffer & sharedVertexBuffer);

    void
    updateVertexSubBuffer(models::VertexBuffer const & vertexBuffer,
                          opengl::SharedVertexSubBuffer & subBuffer);
}
