#pragma once

#include <minire/errors.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/utils/aabb.hpp>

#include <opengl.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>

#include <unordered_map>

namespace minire::opengl
{
    // TODO: this code is a mess, refact it!
    // TODO: maybe rename it? Like Brush or Drawable
    struct VertexBuffer
    {
        using VboMap = std::unordered_map<size_t, opengl::VBO>;

        opengl::VAO::Sptr _vao;
        VboMap            _vboMap;
        size_t            _elementsCount = 0;
        GLenum            _elementsType = 0;
        utils::Aabb       _aabb;
        GLenum            _drawMode = GL_TRIANGLES;

    public:
        VertexBuffer()
            : _vao(std::make_shared<opengl::VAO>())
        {}

    public:
        VertexBuffer(VertexBuffer &&) = default;

        opengl::VBO & createVbo(size_t index, GLenum target)
        {
            auto [it, inserted] = _vboMap.emplace(index, opengl::VBO(_vao, target));
            if (!inserted && it->second.target() != target)
            {
                MINIRE_THROW("VBO re-created w/ different target: {} != {}",
                             target, it->second.target());
            }
            return it->second;
        }

        void bindVao() const { assert(_vao); _vao->bind(); }

        utils::Aabb const & aabb() const { return _aabb; }

        void drawElements() const
        {
            bindVao();
            MINIRE_GL(glDrawElements, _drawMode, _elementsCount, _elementsType, 0);
        }

    private:
        VertexBuffer(VertexBuffer const &) = delete;
        VertexBuffer& operator=(VertexBuffer const &) = delete;
    };
}
