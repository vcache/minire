#include <rasterizer/shared-vertex-buffers.hpp>

#include <rasterizer/materials.hpp>

namespace minire::rasterizer
{
    namespace
    {
        size_t eboElementsPerPrimitive(GLenum const drawMode)
        {
            switch (drawMode)
            {
                case GL_POINTS:
                    return 1;
                case GL_LINES:
                case GL_LINE_STRIP:
                case GL_LINE_LOOP:
                    return 2;
                case GL_TRIANGLES:
                case GL_TRIANGLE_STRIP:
                case GL_TRIANGLE_FAN:
                    return 3;
                default:
                    return 3;
            }
        }
    }

    size_t SharedVertexBuffers::elementsForBytes(size_t const bytes,
                                                GLenum const drawMode,
                                                size_t const vertexStride)
    {
        size_t const eboCount = eboElementsPerPrimitive(drawMode);
        size_t const bytesPerElement = eboCount * (sizeof(uint32_t) + vertexStride);
        assert(bytesPerElement > 0);
        return bytes / bytesPerElement;
    }

    size_t SharedVertexBuffers::elementsForBytes(size_t const bytes,
                                                models::MeshFeatures const & meshFeatures,
                                                GLenum const drawMode)
    {
        return elementsForBytes(bytes, drawMode,
                                opengl::SharedVertexBuffer::calculateVerticesStride(meshFeatures));
    }

    SharedVertexBuffers::SharedVertexBuffers(Materials const & materials,
                                             size_t const maxElements)
        : _materials(materials)
        , _maxElements(maxElements)
    {}

    opengl::SharedVertexBuffer::Sptr const &
    SharedVertexBuffers::get(models::MeshFeatures const & meshFeatures,
                             GLenum const drawMode,
                             GLenum const target)
    {
        Key const key
        {
            ._drawMode = drawMode,
            ._meshFeatures = meshFeatures,
            ._target = target,
        };

        opengl::SharedVertexBuffer::Sptr & result = _store[key];
        if (!result)
        {
            result = std::make_shared<opengl::SharedVertexBuffer>(drawMode,
                                                                  _materials.locations(),
                                                                  meshFeatures,
                                                                  _maxElements,
                                                                  target);
        }
        return result;
    }
}
