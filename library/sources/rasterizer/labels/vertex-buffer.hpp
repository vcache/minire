#pragma once

#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <cassert>
#include <optional>
#include <vector>

namespace minire::rasterizer { class Font; }
namespace minire::text { class FormattedString; }
namespace minire::text { class Format; }

namespace minire::rasterizer::labels
{
    // NOTE: Clients must align data elements consistent with the
    //       requirements of the client platform, with an additional
    //       base-level requirement that an offset within a buffer to
    //       a datum comprising N be a multiple of N.
    struct Vertex
    {
        // TODO: add alignas
        glm::vec2 _pos;
        glm::vec2 _uv;
        glm::vec4 _fgColor;
        glm::vec4 _bgColor;
        uint32_t  _font;
    };

    std::vector<Vertex> buildMesh(text::FormattedString const & text,
                                  Font const & fontRegular,
                                  Font const & fontBold,
                                  Font const & fontItalic);

    // In Vertex shader:
    //    layout(location = 0) in vec2 bznkPos;
    //    layout(location = 1) in vec2 bznkUv;
    //    layout(location = 2) in vec4 bznkFgColor;
    //    layout(location = 3) in vec4 bznkBgColor;
    //    layout(location = 4) in uint bznkFont; // [0; 3)
    class VertexBuffer
    {
    public:
        explicit VertexBuffer(std::vector<Vertex> const &);

        void draw() const
        {
            _vao.bind();
            MINIRE_GL(glDrawArrays, GL_TRIANGLES, 0, _vertices);
        }

    private:
        opengl::VAO  _vao;
        opengl::VBO  _vbo;
        size_t const _vertices;
    };
}
