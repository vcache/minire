#include <rasterizer/sprites/vertex-buffer.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <utils/overloaded.hpp>

#include <cassert>
#include <cmath>

namespace minire::rasterizer::sprites
{
    namespace
    {
        size_t makeQuad(glm::vec2 const & quadOffset, float width, float height,
                        float left, float top, float right, float bottom,
                        size_t offset, std::vector<Vertex> & out)
        {
            if (!(width >= 0 && height >= 0 &&
                  left <= right && top <= bottom))
            {
                MINIRE_WARNING("bad quad: width = {}; height = {}; "
                               "left = {}; right = {}; top = {}; bottom = {}; ",
                               width, height, left, right, top, bottom);
                return 0;
            }

            glm::vec2 const texMinSz(right - left + 1.0f, bottom - top + 1.0f);
            glm::vec2 const repeat = glm::vec2(width, height) / texMinSz;

            out[offset + 0]._pos = floor(quadOffset + glm::vec2(0.0, height));
            out[offset + 1]._pos = floor(quadOffset + glm::vec2(0.0, 0.0));
            out[offset + 2]._pos = floor(quadOffset + glm::vec2(width, 0.0));
            out[offset + 3]._pos = floor(quadOffset + glm::vec2(0.0, height));
            out[offset + 4]._pos = floor(quadOffset + glm::vec2(width, 0.0));
            out[offset + 5]._pos = floor(quadOffset + glm::vec2(width, height));

            out[offset + 0]._uv = glm::vec2(0, 1) * repeat;
            out[offset + 1]._uv = glm::vec2(0, 0) * repeat;
            out[offset + 2]._uv = glm::vec2(1, 0) * repeat;
            out[offset + 3]._uv = glm::vec2(0, 1) * repeat;
            out[offset + 4]._uv = glm::vec2(1, 0) * repeat;
            out[offset + 5]._uv = glm::vec2(1, 1) * repeat;

            for(int i(0); i < 6; ++i)
            {
                out[offset + i]._rep = glm::vec2(left, top);
                out[offset + i]._dims = texMinSz;
            }

            return 6;
        }

        size_t makeQuad(glm::vec2 const & quadOffset,
                        float left, float top, float right, float bottom,
                        size_t offset, std::vector<Vertex> & out)
        {
            float const width = right - left + 1.0f;
            float const height = bottom - top + 1.0f;
            return makeQuad(quadOffset, width, height,
                            left, top, right, bottom,
                            offset, out);
        }
    }

    std::vector<Vertex> buildMesh(utils::Rect const & tile,
                                  glm::vec2 const & position)
    {
        std::vector<Vertex> result(6);
        makeQuad(position, tile._left, tile._top, tile._right, tile._bottom,
                 0, result);
        return result;
    }

    // TODO: parts are overlapping at +- 1 pixel
    std::vector<Vertex> buildMesh(utils::NinePatch const & tile,
                                  glm::vec2 const & position,
                                  glm::vec2 const & dimensions)
    {
        std::vector<Vertex> result(6*9);
        size_t offset = 0;
        
        // corners (boundary - out) //

        glm::vec2 const corners(dimensions.x - (tile._boundary._right - tile._out._right + 1.0f),
                                dimensions.y - (tile._out._top - tile._boundary._top + 1.0f));

        // III
        offset += makeQuad(
            position + glm::vec2(0, corners.y),
            tile._boundary._left,
            tile._out._bottom,
            tile._out._left,
            tile._boundary._bottom,
            offset, result);

        // IV
        offset += makeQuad(
            position + corners,
            tile._out._right,
            tile._out._bottom,
            tile._boundary._right,
            tile._boundary._bottom,
            offset, result);

        // I
        offset += makeQuad(
            position,
            tile._boundary._left,
            tile._boundary._top,
            tile._out._left,
            tile._out._top,
            offset, result);

        // II
        offset += makeQuad(
            position + glm::vec2(corners.x, 0),
            tile._out._right,
            tile._boundary._top,
            tile._boundary._right,
            tile._out._top,
            offset, result);

        // borders (out - in) //

        // bottom
        offset += makeQuad(
            position + glm::vec2(tile._out._left - tile._boundary._left + 1.0f,
                                 dimensions.y - (tile._out._top - tile._boundary._top + 1.0f)),

            dimensions.x - (tile._out._left - tile._boundary._left + 1.0f)
                         - (tile._boundary._right - tile._out._right + 1.0f),
            tile._out._top - tile._boundary._top + 1.0f,

            tile._in._left, tile._out._bottom,
            tile._in._right, tile._boundary._bottom,
            offset, result);

        // left
        offset += makeQuad(
            position + glm::vec2(0, tile._out._top - tile._boundary._top + 1.0f),

            tile._out._left - tile._boundary._left + 1.0f,
            dimensions.y - (tile._out._top - tile._boundary._top + 1.0f)
                         - (tile._boundary._bottom - tile._out._bottom + 1.0f),

            tile._boundary._left, tile._in._top,
            tile._out._right, tile._in._bottom,
            offset, result);

        // right
        offset += makeQuad(
            position + glm::vec2(dimensions.x - (tile._boundary._right - tile._out._right + 1.0f),
                                 tile._out._top - tile._boundary._top + 1.0f),

            tile._out._left - tile._boundary._left + 1.0f,
            dimensions.y - (tile._out._top - tile._boundary._top + 1.0f)
                         - (tile._boundary._bottom - tile._out._bottom + 1.0f),

            tile._out._right, tile._in._top,
            tile._boundary._right, tile._in._bottom,
            offset, result);

        // top
        offset += makeQuad(
            position + glm::vec2(tile._out._left - tile._boundary._left + 1.0f, 0),

            dimensions.x - (tile._out._left - tile._boundary._left + 1.0f)
                         - (tile._boundary._right - tile._out._right + 1.0f),
            tile._out._top - tile._boundary._top + 1.0f,

            tile._in._left, tile._boundary._top,
            tile._in._right, tile._out._top,
            offset, result);

        // center //

        offset += makeQuad(
            position + glm::vec2((tile._out._left - tile._boundary._left + 1.0f),
                                 (tile._out._top - tile._boundary._top + 1.0f)),

            dimensions.x - (tile._out._left - tile._boundary._left + 1.0f)
                         - (tile._boundary._right - tile._out._right + 1.0f),
            dimensions.y - (tile._out._top - tile._boundary._top + 1.0f)
                         - (tile._boundary._bottom - tile._out._bottom + 1.0f),

            tile._in._left, tile._in._top,
            tile._in._right, tile._in._bottom,
            offset, result);

        assert(offset <= result.size());
        return result;
    }

    std::vector<Vertex> buildMesh(utils::Patch const & patch,
                                  glm::vec2 const & position,
                                  glm::vec2 const & dimensions,
                                  glm::vec2 const & textureSize)
    {
        return std::visit(utils::Overloaded
        {
            [&](std::monostate const &)
            {
                utils::Rect const rect(0, 0, textureSize.x - 1, textureSize.y - 1);
                return buildMesh(rect, position);
            },
            [&](utils::Rect const & rect) { return buildMesh(rect, position); },
            [&](utils::NinePatch const & tile) { return buildMesh(tile, position, dimensions); }
        }, patch);
    }

    // VertexBuffer

    VertexBuffer::VertexBuffer(std::vector<Vertex> const & vertices)
        : _vao(std::make_shared<opengl::VAO>())
        , _vbo(_vao, GL_ARRAY_BUFFER)
        , _vertices(vertices.size())
    {
        size_t const stride = sizeof(Vertex);
        size_t pointer = 0;

        // layout(location = 0) in vec2 bznkPos;
        _vao->enableAttrib(0);
        _vao->attribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, pointer);
        pointer += sizeof(Vertex::_pos);

        // layout(location = 1) in vec2 bznkUv;
        _vao->enableAttrib(1);
        _vao->attribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, pointer);
        pointer += sizeof(Vertex::_uv);

        // layout(location = 2) in vec2 bznkRep;
        _vao->enableAttrib(2);
        _vao->attribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, pointer);
        pointer += sizeof(Vertex::_rep);

        // layout(location = 3) in vec2 bznkDims;
        _vao->enableAttrib(3);
        _vao->attribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, pointer);
        pointer += sizeof(Vertex::_rep);

        // allocate storage
        _vbo.bufferData(vertices.size() * sizeof(Vertex),
                        vertices.data(), GL_STATIC_DRAW);
    }

    void VertexBuffer::reload(std::vector<Vertex> const & vertices)
    {
        MINIRE_INVARIANT(vertices.size() == _vertices,
                         "cannot change size of GL_ARRAY_BUFFER from {} to {}",
                         _vertices, vertices.size());
        _vbo.bufferSubData(0, vertices.size() * sizeof(Vertex),
                              vertices.data());
    }
}