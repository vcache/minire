#pragma once

#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>

#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

#include <cassert>
#include <vector>

namespace minire::rasterizer::sprites
{
    // NOTE: Clients must align data elements consistent with the 
    //       requirements of the client platform, with an additional
    //       base-level requirement that an offset within a buffer to
    //       a datum comprising N be a multiple of N.        
    struct Vertex
    {
        // TODO: why 2?
        alignas(2) glm::vec2 _pos;
        alignas(2) glm::vec2 _uv;
        alignas(2) glm::vec2 _rep;
        alignas(2) glm::vec2 _dims;
    };

    std::vector<Vertex> buildMesh(utils::Rect const &,
                                  glm::vec2 const & position);

    std::vector<Vertex> buildMesh(utils::NinePatch const &,
                                  glm::vec2 const & position,
                                  glm::vec2 const & dimensions);

    // all valueas are in pixels
    std::vector<Vertex> buildMesh(utils::Patch const &,
                                  glm::vec2 const & position,
                                  glm::vec2 const & dimensions,     // used only for NinePatch
                                  glm::vec2 const & textureSize);

    // layout(location = 0) in vec2 bznkPos;
    // layout(location = 1) in vec2 bznkUv;
    // layout(location = 2) in vec2 bznkRep;
    // layout(location = 3) in vec2 bznkDims;
    class VertexBuffer
    {
    public:
        explicit VertexBuffer(std::vector<Vertex> const &);

        void reload(std::vector<Vertex> const &);

        void draw() const
        {
            assert(_vao);
            _vao->bind();
            MINIRE_GL(glDrawArrays, GL_TRIANGLES, 0, _vertices);
        }

    private:
        opengl::VAO::Sptr _vao;
        opengl::VBO       _vbo;
        size_t const      _vertices;
    };
}