#include <opengl/shared-vertex-buffer.hpp>

#include <material/types.hpp>
#include <minire/errors.hpp>

#include <algorithm>
#include <cassert>

namespace minire::opengl
{
    // SharedVBO::BlockHolder

    SharedVBO::BlockHolder::BlockHolder(SharedVBO & sharedVbo, size_t size)
        : _sharedVbo(sharedVbo.weak_from_this())
        , _block(sharedVbo.allocate(size))
    {}

    SharedVBO::BlockHolder::BlockHolder(BlockHolder && other) noexcept
        : _sharedVbo(std::move(other._sharedVbo))
        , _block(other._block)
    {
        other._sharedVbo.reset();
        other._block = Block{};
    }

    SharedVBO::BlockHolder & SharedVBO::BlockHolder::operator=(BlockHolder && other) noexcept
    {
        BlockHolder tmp(std::move(other));
        std::swap(_sharedVbo, tmp._sharedVbo);
        std::swap(_block, tmp._block);
        return *this;
    }

    SharedVBO::BlockHolder::~BlockHolder()
    {
        release();
    }

    void SharedVBO::BlockHolder::release()
    {
        if (auto sharedVbo = _sharedVbo.lock(); sharedVbo && _block._size > 0)
        {
            sharedVbo->free(_block);
            _block = Block{};
        }
    }

    void SharedVBO::BlockHolder::bufferSubData(GLsizeiptr size,
                                              const void * data)
    {
        assert(!_sharedVbo.expired());
        assert(static_cast<size_t>(size) <= _block._size);
        if (auto sharedVbo = _sharedVbo.lock(); sharedVbo)
        {
            sharedVbo->_vbo.bufferSubData(_block._offset, size, data);
        }
    }

    // SharedVBO

    SharedVBO::SharedVBO(GLenum const target,
                         size_t const maxSize,
                         GLenum const usage)
        : _vbo(target)
    {
        _vbo.bufferData(maxSize, nullptr /* allocate only */, usage);
    }

    SharedVBO::Block SharedVBO::allocate(size_t const size)
    {
        // look up in an empty elements
        for(auto it = _freeBlocks.begin(); it != _freeBlocks.end(); ++it)
        {
            if (it->_size >= size)
            {
                Block result{it->_offset, size};
                Block remaining{it->_offset + size, it->_size - size};
                _freeBlocks.erase(it);
                if (remaining._size > 0)
                {
                    _freeBlocks.insert(remaining);
                }
                return result;
            }
        }

        // ensure VBO's capacity won't be exceeded
        MINIRE_INVARIANT(_nextOffset + size <= static_cast<size_t>(_vbo.size()),
                         "not enough SharedVBO size: {} bytes", _vbo.size());

        // allocate a new block
        Block result
        {
            ._offset = _nextOffset,
            ._size = size,
        };
        _nextOffset += size;
        return result;
    }

    void SharedVBO::free(SharedVBO::Block block)
    {
        if (block._size == 0) return;

        if (block._offset + block._size == _nextOffset)
        {
            // special case: freeing the last block
            assert(_nextOffset >= block._size);
            _nextOffset -= block._size;
        }
        else
        {
            _freeBlocks.insert(block);
        }
    }

    // SharedVertexBuffer

    namespace
    {
        size_t vboSize(models::MeshFeatures const & meshFeatures)
        {
            size_t values = 3; // (x, y, z)
            if (meshFeatures.hasUv()) values += 2; // [u, v]
            if (meshFeatures.hasNormal()) values += 3; // [nx, ny, nz]
            if (meshFeatures.hasTangent()) values += 3; // [tx, ty, tz]
            if (meshFeatures.hasSkin()) values += 4 + 4; // joints (uvec4), weights (vec4)
            return values;
        }

        size_t eboSize(GLenum drawMode)
        {
            switch(drawMode)
            {
                case GL_POINTS:    return 1; // (A)
                case GL_LINES:     return 2; // (A, B)
                case GL_TRIANGLES: return 3; // (A, B, C)
                default:           MINIRE_THROW("unsupported draw mode: {}",
                                                static_cast<int>(drawMode));
            }
        }
    }

    size_t SharedVertexBuffer::calculateVerticesStride(models::MeshFeatures const & meshFeatures)
    {
        return vboSize(meshFeatures) * sizeof(float);
    }

    SharedVertexBuffer::SharedVertexBuffer(GLenum const drawMode,
                                           material::Locations const & locations,
                                           models::MeshFeatures const & meshFeatures,
                                           size_t const maxElements,
                                           GLenum const target)
        : _drawMode(drawMode)
        , _meshFeatures(meshFeatures)
        , _maxElements(maxElements)
        , _indecesStride(eboSize(drawMode) * sizeof(uint32_t))
        , _verticesStride(calculateVerticesStride(_meshFeatures))
        , _vao()
        , _elements(std::make_shared<SharedVBO>(GL_ELEMENT_ARRAY_BUFFER, _maxElements * _indecesStride, target))
        , _vertices(std::make_shared<SharedVBO>(GL_ARRAY_BUFFER, _maxElements * eboSize(drawMode) * _verticesStride, target))
    {
        _vao.bind();
        _elements->bind(); // ensures VAO associates with element array buffer
        _vertices->bind();

        // attributes helper
        size_t pointer = 0;
        auto setupAttrib = [&pointer, this](int const attribIndex,
                                            size_t const components,
                                            bool const isPresent)
        {
            if (isPresent)
            {
                if (attribIndex != -1)
                {
                    _vao.enableAttrib(attribIndex);
                    _vao.attribPointer(attribIndex, components, GL_FLOAT, GL_FALSE, _verticesStride, pointer);
                }
                pointer += (components * sizeof(float));
            }
        };

        // setup attribs
        setupAttrib(locations.vertexAttribute(), 3, true);
        setupAttrib(locations.uvAttribute(), 2, meshFeatures.hasUv());
        setupAttrib(locations.normalAttribute(), 3, meshFeatures.hasNormal());
        setupAttrib(locations.tangentAttribute(), 3, meshFeatures.hasTangent());
        if (meshFeatures.hasSkin())
        {
            if (locations.jointsAttribute() != -1)
            {
                _vao.enableAttrib(locations.jointsAttribute());
                _vao.attribIPointer(locations.jointsAttribute(), 4, GL_UNSIGNED_INT, _verticesStride, pointer);
            }
            pointer += (4 * sizeof(uint32_t));

            if (locations.weightsAttribute() != -1)
            {
                _vao.enableAttrib(locations.weightsAttribute());
                _vao.attribPointer(locations.weightsAttribute(), 4, GL_FLOAT, GL_FALSE, _verticesStride, pointer);
            }
            pointer += (4 * sizeof(float));
        }
        assert(pointer == _verticesStride);

        VAO::unbind();
    }

    std::unique_ptr<SharedVertexSubBuffer>
    SharedVertexBuffer::create(size_t const elementsCount, utils::Aabb const & aabb,
                               size_t const eboBytes, void const * eboData,
                               size_t const vboBytes, void const * vboData)
    {
        VAO::unbind();

        auto multiBlock = allocate(eboBytes, vboBytes, elementsCount);

        assert(eboData);
        assert(eboBytes % sizeof(uint32_t) == 0);
        multiBlock._elements.bufferSubData(eboBytes, eboData);

        assert(vboData);
        assert(vboBytes % _verticesStride == 0);
        multiBlock._vertices.bufferSubData(vboBytes, vboData);

        return std::unique_ptr<SharedVertexSubBuffer>(
            new SharedVertexSubBuffer(*this, std::move(multiBlock), aabb));
    }

    SharedVertexBuffer::MultiBlock
    SharedVertexBuffer::allocate(size_t eboBytes, size_t vboBytes, size_t elementsCount)
    {
        assert(_vertices);
        assert(_elements);
        return MultiBlock
        {
            ._elements = SharedVBO::BlockHolder(*_elements, eboBytes),
            ._vertices = SharedVBO::BlockHolder(*_vertices, vboBytes),
            ._elementsCount = elementsCount,
        };
    }

    // SharedVertexSubBuffer

    SharedVertexSubBuffer::SharedVertexSubBuffer(SharedVertexBuffer & sharedVertexBuffer,
                                                 SharedVertexBuffer::MultiBlock && multiBlock,
                                                 utils::Aabb const & aabb)
        : _sharedVertexBuffer(sharedVertexBuffer.weak_from_this())
        , _multiBlock(std::move(multiBlock))
        , _aabb(aabb)
        , _commandData
            {
                ._mode = sharedVertexBuffer.drawMode(),
                ._elementCount = _multiBlock._elementsCount,
                ._elementOffset = _multiBlock._elements.offset() / sizeof(uint32_t),
                ._baseVertex = static_cast<int32_t>(_multiBlock._vertices.offset() / sharedVertexBuffer.verticesStride()),
            }
    {}
}
