#pragma once

#include <opengl.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>

#include <minire/utils/aabb.hpp>
#include <minire/models/mesh-features.hpp>

#include <cassert>
#include <memory>
#include <set>
#include <tuple>
#include <utility>

namespace minire::material { class Locations; }

namespace minire::opengl
{
    // SharedVBO,
    // A single VBO (can be used for vertices, indeces, uvs, etc)

    class SharedVBO
        : public std::enable_shared_from_this<SharedVBO>
    {
        SharedVBO(SharedVBO const &) = delete;
        SharedVBO & operator=(SharedVBO const &) = delete;

    public:
        explicit SharedVBO(GLenum const target,
                           size_t const maxSize, // in bytes
                           GLenum const usage = GL_STATIC_DRAW);

    public:
        class BlockHolder
        {
            BlockHolder(BlockHolder const &) = delete;
            BlockHolder & operator=(BlockHolder const &) = delete;

        public:
            explicit BlockHolder(SharedVBO & sharedVbo, size_t size);

            BlockHolder(BlockHolder && other) noexcept;
            BlockHolder & operator=(BlockHolder && other) noexcept;

            ~BlockHolder();

        public:
            void bufferSubData(GLsizeiptr size,
                               const void * data);

            size_t offset() const { assert(!_sharedVbo.expired()); return _block._offset; }
            size_t size() const { assert(!_sharedVbo.expired()); return _block._size; }

        private:
            struct Block
            {
                size_t _offset = 0; // bytes
                size_t _size = 0;   // bytes

                bool operator<(Block const & o) const
                {
                    return std::tie(_size, _offset) <
                           std::tie(o._size, o._offset);
                }
            };

            std::weak_ptr<SharedVBO> _sharedVbo;
            Block                    _block;

            friend class SharedVBO;
        };

        void bind() const { _vbo.bind(); }

    private:
        using Block = BlockHolder::Block;

        Block allocate(size_t size); // bytes
        void free(Block block);

    private:
        VBO             _vbo;
        size_t          _nextOffset = 0; // in bytes
        std::set<Block> _freeBlocks;

        friend class BlockHolder;
    };

    // SharedVertexBuffer (A collection of VBOs)

    class SharedVertexSubBuffer;

    class SharedVertexBuffer
        : public std::enable_shared_from_this<SharedVertexBuffer>
    {
        SharedVertexBuffer(SharedVertexBuffer const &) = delete;
        SharedVertexBuffer& operator=(SharedVertexBuffer const &) = delete;
        SharedVertexBuffer(SharedVertexBuffer &&) = delete;
        SharedVertexBuffer& operator=(SharedVertexBuffer &&) = delete;

    public:
        explicit SharedVertexBuffer(GLenum const drawMode,
                                    material::Locations const & locations,
                                    models::MeshFeatures const & meshFeatures,
                                    size_t const maxElements,
                                    GLenum const target = GL_STATIC_DRAW);

        using Sptr = std::shared_ptr<SharedVertexBuffer>;
        using Wptr = std::weak_ptr<SharedVertexBuffer>;

        static size_t calculateVerticesStride(models::MeshFeatures const & meshFeatures);

    public:
        VAO & vao() { return _vao; }
        VAO const & vao() const { return _vao; }

    public:
        GLenum drawMode() const { return _drawMode; }
        GLenum eboType() const { return GL_UNSIGNED_INT; }
        GLenum elementsType() const { return GL_UNSIGNED_INT; }
        size_t stride() const { return _verticesStride / sizeof(float); }
        size_t indecesStride() const { return _indecesStride; }
        size_t verticesStride() const { return _verticesStride; }
        models::MeshFeatures const & meshFeatures() const { return _meshFeatures; }

        void bindElements() const { _elements->bind(); }

        struct MultiBlock
        {
            SharedVBO::BlockHolder _elements;
            SharedVBO::BlockHolder _vertices;
            size_t                 _elementsCount = 0;
        };

        std::unique_ptr<SharedVertexSubBuffer>
        create(size_t const elementsCount, utils::Aabb const &,
               size_t const eboBytes, void const * eboData,
               size_t const vboBytes, void const * vboData);

        void update(SharedVertexSubBuffer & subBuffer,
                    size_t const elementsCount, utils::Aabb const &,
                    size_t const eboBytes, void const * eboData,
                    size_t const vboBytes, void const * vboData);

    private:
        MultiBlock allocate(size_t eboBytes, size_t vboBytes, size_t elementsCount);

    private:
        GLenum const               _drawMode;
        models::MeshFeatures const _meshFeatures;
        size_t const               _maxElements;
        size_t const               _indecesStride;
        size_t const               _verticesStride;
        VAO                        _vao;
        std::shared_ptr<SharedVBO> _elements;   // GL_ELEMENT_ARRAY_BUFFER
        std::shared_ptr<SharedVBO> _vertices;   // GL_ARRAY_BUFFER

        friend class SharedVertexSubBuffer;
    };

    // SharedVertexSubBuffer

    class SharedVertexSubBuffer
    {
        SharedVertexSubBuffer(SharedVertexBuffer const &) = delete;
        SharedVertexSubBuffer& operator=(SharedVertexBuffer const &) = delete;
        SharedVertexSubBuffer(SharedVertexBuffer &&) = delete;
        SharedVertexSubBuffer& operator=(SharedVertexBuffer &&) = delete;

    private:
        explicit SharedVertexSubBuffer(SharedVertexBuffer & sharedVertexBuffer,
                                       SharedVertexBuffer::MultiBlock && multiBlock,
                                       utils::Aabb const & aabb);

    public:
        using Uptr = std::unique_ptr<SharedVertexSubBuffer>;

        // NOTE: 'type' is always GL_UNSIGNED_INT
        struct CommandData
        {
            GLenum  _mode;           // usually GL_TRIANGLES
            size_t  _elementCount;   // number of elements
            size_t  _elementOffset;  // an index (not byte) of the first element in GL_ELEMENT_ARRAY_BUFFER
            int32_t _baseVertex;     // an index of the first vertex in GL_ARRAY_BUFFER
        };

        CommandData const & commandData() const
        {
            assert(!_sharedVertexBuffer.expired());
            return _commandData;
        }

        utils::Aabb const & aabb() const
        {
            assert(!_sharedVertexBuffer.expired());
            return _aabb;
        }

        SharedVertexBuffer::Sptr sharedVertexBuffer() const
        {
            return _sharedVertexBuffer.lock();
        }

        void update(SharedVertexBuffer::MultiBlock && multiBlock,
                    utils::Aabb const & aabb);

    private:
        SharedVertexBuffer::Wptr       _sharedVertexBuffer;
        SharedVertexBuffer::MultiBlock _multiBlock;
        utils::Aabb                    _aabb;
        CommandData                    _commandData;

        friend class SharedVertexBuffer;
    };
}
