#pragma once

#include <minire/utils/aabb.hpp>

#include <opengl.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>

#include <vector>

namespace minire::opengl
{
    // TODO: this code is a mess, refact it!
    //
    struct IndexBuffers
    {
        // TODO: why not a std::bitset?
        static constexpr size_t kHaveNormals  = (1 << 0);
        static constexpr size_t kHaveTangents = (1 << 1);
        static constexpr size_t kHaveUvs      = (1 << 2);

        opengl::VAO::Sptr _vao;
        opengl::VBO       _ebo;
        opengl::VBO       _vbo;
        size_t            _indeces = 0;
        size_t            _flags = 0;
        utils::Aabb       _aabb;

    public:
        IndexBuffers(size_t indeces,
                     utils::Aabb aabb)
            : _vao(std::make_shared<opengl::VAO>())
            , _ebo(opengl::VBO(_vao, GL_ELEMENT_ARRAY_BUFFER))
            , _vbo(opengl::VBO(_vao, GL_ARRAY_BUFFER))
            , _indeces(indeces)
            , _aabb(aabb)
        {}

    public:
        IndexBuffers(IndexBuffers &&) = default;

        auto flags() const { return _flags; }

        bool hasUvs() const { return _flags & kHaveUvs; }

        bool hasNormals() const { return _flags & kHaveNormals; }

        bool hasTangents() const { return _flags & kHaveTangents; }

        void bindVao() const { assert(_vao); _vao->bind(); }

        utils::Aabb const & aabb() const { return _aabb; }

        void drawElements() const
        {
            bindVao();
            MINIRE_GL(glDrawElements, GL_TRIANGLES, _indeces, GL_UNSIGNED_INT, 0);
        }

    private:
        IndexBuffers(IndexBuffers const &) = delete;
        IndexBuffers& operator=(IndexBuffers const &) = delete;
    };
}
