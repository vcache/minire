#pragma once

#include <minire/errors.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/utils/aabb.hpp>

#include <opengl.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>

#include <utility>
#include <unordered_map>

namespace minire::opengl
{
    // TODO: this code is a mess, refact it!
    // TODO: maybe rename it? Like Brush or Drawable
    struct VertexBuffer
    {
        using VboMap = std::unordered_map<size_t, opengl::VBO>;

        opengl::VAO::Sptr _vao; // TODO: why not unique_ptr?
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

        opengl::VBO * findVbo(size_t index)
        {
            auto it = _vboMap.find(index);
            return it != _vboMap.end() ? &it->second : nullptr;
        }

        void bindVao() const { assert(_vao); _vao->bind(); }

        utils::Aabb const & aabb() const { return _aabb; }

        void drawElements() const
        {
            bindVao();
            MINIRE_GL(glDrawElements, _drawMode, _elementsCount, _elementsType, 0);
        }

        VertexBuffer& operator=(VertexBuffer && other)
        {
            VertexBuffer tmp(std::move(other));
            std::swap(tmp._vao, _vao);
            std::swap(tmp._vboMap, _vboMap);
            std::swap(tmp._elementsCount, _elementsCount);
            std::swap(tmp._elementsType, _elementsType);
            std::swap(tmp._aabb, _aabb);
            std::swap(tmp._drawMode, _drawMode);
            return *this;
        }

    private:
        VertexBuffer(VertexBuffer const &) = delete;
        VertexBuffer& operator=(VertexBuffer const &) = delete;
    };
}
