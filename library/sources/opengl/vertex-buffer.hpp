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

        opengl::VAO _vao;
        VboMap      _vboMap;
        size_t      _elementsCount = 0;
        GLenum      _elementsType = 0;
        utils::Aabb _aabb;
        GLenum      _drawMode = GL_TRIANGLES;
        bool        _doubleSided = false;

    public:
        VertexBuffer() = default;

    public:
        VertexBuffer(VertexBuffer &&) = default;

        opengl::VBO & createVbo(size_t index, GLenum target)
        {
            if (GL_ELEMENT_ARRAY_BUFFER == target)
            {
                // NOTE: EBO belongs to VAO, so that, target's VAO must be bound
                _vao.bind();
            }

            auto [it, inserted] = _vboMap.emplace(index, opengl::VBO(target));
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

        void bindVao() const { _vao.bind(); }

        utils::Aabb const & aabb() const { return _aabb; }

        void drawElements() const
        {
            bindVao();
            // TODO: sort VertexBuffers via doubleSided to avoid frequent context switches
            if (_doubleSided)
            {
                MINIRE_GL(glDisable, GL_CULL_FACE);
            }
            else
            {
                MINIRE_GL(glEnable, GL_CULL_FACE);
            }
            MINIRE_GL(glDrawElements, _drawMode, _elementsCount, _elementsType, 0);
        }

        void drawElementsInstanced(size_t const instancesCount) const
        {
            bindVao();
            // TODO: sort VertexBuffers via doubleSided to avoid frequent context switches
            if (_doubleSided)
            {
                MINIRE_GL(glDisable, GL_CULL_FACE);
            }
            else
            {
                MINIRE_GL(glEnable, GL_CULL_FACE);
            }
            MINIRE_GL(glDrawElementsInstanced, _drawMode, _elementsCount, _elementsType, 0, instancesCount);
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
