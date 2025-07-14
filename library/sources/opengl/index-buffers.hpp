#pragma once

#include <minire/errors.hpp>
#include <minire/utils/aabb.hpp>

#include <opengl.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>

#include <unordered_map>

namespace minire::opengl
{
    // TODO: this code is a mess, refact it!
    // TODO: rename to Drawable
    //
    struct IndexBuffers
    {
        // TODO: why not a std::bitset?
        static constexpr size_t kHaveNormals  = (1 << 0);
        static constexpr size_t kHaveTangents = (1 << 1);
        static constexpr size_t kHaveUvs      = (1 << 2);

        using VboMap = std::unordered_map<size_t, opengl::VBO>;

        opengl::VAO::Sptr _vao;
        VboMap            _vboMap;
        size_t            _elementsCount = 0;
        GLenum            _elementsType = 0;
        size_t            _flags = 0;
        utils::Aabb       _aabb;
        GLenum            _drawMode = GL_TRIANGLES;

    public:
        IndexBuffers()
            : _vao(std::make_shared<opengl::VAO>())
        {}

    public:
        IndexBuffers(IndexBuffers &&) = default;

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

        auto flags() const { return _flags; }

        bool hasUvs() const { return _flags & kHaveUvs; }

        bool hasNormals() const { return _flags & kHaveNormals; }

        bool hasTangents() const { return _flags & kHaveTangents; }

        void bindVao() const { assert(_vao); _vao->bind(); }

        utils::Aabb const & aabb() const { return _aabb; }

        void drawElements() const
        {
            bindVao();
            MINIRE_GL(glDrawElements, _drawMode, _elementsCount, _elementsType, 0);
        }

    private:
        IndexBuffers(IndexBuffers const &) = delete;
        IndexBuffers& operator=(IndexBuffers const &) = delete;
    };
}
