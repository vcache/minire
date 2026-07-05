#include <utils/vertex-buffer-builder.hpp>

#include <minire/errors.hpp>
#include <minire/models/vertex-buffer.hpp>
#include <minire/utils/always-false.hpp>
#include <minire/utils/overloaded.hpp>

#include <material/types.hpp>
#include <opengl/vertex-buffer.hpp>

#include <cassert>

namespace minire::utils
{
    namespace
    {
        GLenum convertDrawMode(models::VertexBuffer::DrawMode drawMode)
        {
            switch(drawMode)
            {
                case models::VertexBuffer::DrawMode::kLines:         return GL_LINES;
                case models::VertexBuffer::DrawMode::kLineStrip:     return GL_LINE_STRIP;
                case models::VertexBuffer::DrawMode::kLineLoop:      return GL_LINE_LOOP;
                case models::VertexBuffer::DrawMode::kTriangles:     return GL_TRIANGLES;
                case models::VertexBuffer::DrawMode::kTriangleStrip: return GL_TRIANGLE_STRIP;
                case models::VertexBuffer::DrawMode::kTriangleFan:   return GL_TRIANGLE_FAN;
                case models::VertexBuffer::DrawMode::kQuads:         return GL_QUADS;
                case models::VertexBuffer::DrawMode::kQuadStrip:     return GL_QUAD_STRIP;
            }
            MINIRE_THROW("unknown draw mode: {}", static_cast<int>(drawMode));
        }

        GLenum convertUsage(models::VertexBuffer::Usage usage)
        {
            switch(usage)
            {
                case models::VertexBuffer::Usage::kStreamDraw:  return GL_STREAM_DRAW;
                case models::VertexBuffer::Usage::kStreamRead:  return GL_STREAM_READ;
                case models::VertexBuffer::Usage::kStreamCopy:  return GL_STREAM_COPY;
                case models::VertexBuffer::Usage::kStaticDraw:  return GL_STATIC_DRAW;
                case models::VertexBuffer::Usage::kStaticRead:  return GL_STATIC_READ;
                case models::VertexBuffer::Usage::kStaticCopy:  return GL_STATIC_COPY;
                case models::VertexBuffer::Usage::kDynamicDraw: return GL_DYNAMIC_DRAW;
                case models::VertexBuffer::Usage::kDynamicRead: return GL_DYNAMIC_READ;
                case models::VertexBuffer::Usage::kDynamicCopy: return GL_DYNAMIC_COPY;
            }
            MINIRE_THROW("unknown draw mode: {}", static_cast<int>(usage));
        }

        template<typename T>
        void loadBufferData(models::VertexBuffer::BufferSptr<T> const buffer,
                            opengl::VBO & vbo, GLenum usage)
        {
            assert(buffer);
            vbo.bufferData(buffer->size() * sizeof(T), buffer->data(), usage);
        }

        GLenum bufferElementType(models::VertexBuffer::DataType dataType)
        {
            switch(dataType)
            {
                case models::VertexBuffer::DataType::kInt8:     return GL_BYTE;
                case models::VertexBuffer::DataType::kUInt8:    return GL_UNSIGNED_BYTE;
                case models::VertexBuffer::DataType::kInt16:    return GL_SHORT;
                case models::VertexBuffer::DataType::kUInt16:   return GL_UNSIGNED_SHORT;
                case models::VertexBuffer::DataType::kInt32:    return GL_INT;
                case models::VertexBuffer::DataType::kUInt32:   return GL_UNSIGNED_INT;
                case models::VertexBuffer::DataType::kFloat:    return GL_FLOAT;
                case models::VertexBuffer::DataType::kDouble:   return GL_DOUBLE;
            }
            MINIRE_THROW("unknown data type: {}", static_cast<int>(dataType));
        }

        template<typename T>
        GLenum bufferElementType(models::VertexBuffer::BufferSptr<T> const &)
        {
            if constexpr (std::is_same_v<T, int8_t>) return GL_BYTE;
            else if constexpr (std::is_same_v<T, uint8_t>) return GL_UNSIGNED_BYTE;
            else if constexpr (std::is_same_v<T, int16_t>) return GL_SHORT;
            else if constexpr (std::is_same_v<T, uint16_t>) return GL_UNSIGNED_SHORT;
            else if constexpr (std::is_same_v<T, int32_t>) return GL_INT;
            else if constexpr (std::is_same_v<T, uint32_t>) return GL_UNSIGNED_INT;
            else if constexpr (std::is_same_v<T, float>) return GL_FLOAT;
            else if constexpr (std::is_same_v<T, double>) return GL_DOUBLE;
            else
            {
                static_assert(utils::kAlwaysFalse<T>::value, "unexpected element type");
            }
        }

        GLenum elementType(models::VertexBuffer::TightlyPackedBuffer const & buffer)
        {
            return std::visit([](auto const & i) { return bufferElementType(i); },
                              buffer._byteBuffer);
        }
    }

    std::shared_ptr<opengl::VertexBuffer>
    createVertexBuffer(models::VertexBuffer const & vertexBuffer,
                       material::Locations const & locations)
    {
        auto result = std::make_shared<opengl::VertexBuffer>();

        // draw mode
        result->_drawMode = convertDrawMode(vertexBuffer._drawMode);

        // elements and indeces (EBO)
        opengl::VBO & ebo = result->createVbo(0, GL_ELEMENT_ARRAY_BUFFER);
        std::visit(Overloaded
        {
            [](std::monostate) { MINIRE_THROW("draw arrays mode isn't supported, _elements required"); },
            [&result, &ebo, usage = vertexBuffer._elements._usage](auto const & buffer)
            {
                using Type = std::decay_t<decltype(buffer)>;
                MINIRE_INVARIANT(buffer, "_elements buffer pointer is nullptr");
                result->_elementsCount = buffer->size();
                result->_elementsType = bufferElementType(buffer);
                ebo.bufferData(buffer->size() * sizeof(typename Type::element_type::value_type),
                               buffer->data(), convertUsage(usage));
            }
        }, vertexBuffer._elements._indecesBuffer);

        // VBOs and Attribs
        MINIRE_INVARIANT(!std::holds_alternative<std::monostate>(vertexBuffer._vertices),
                         "_vertices are mandatory, but isn't provided");

        static_assert(sizeof(size_t) >= sizeof(uintptr_t),
                      "unexpected size of a pointer, it is larger than ULL");

        auto loadVertexBuffer = [&result]
                                (models::VertexBuffer::Buffer const & buffer, int attrIndex, char const * hint)
        {
            bool isMonostate = std::holds_alternative<std::monostate>(buffer);
            MINIRE_INVARIANT(isMonostate || attrIndex >= 0, "bad attrIndex = {} for {}, "
                             "the material doesn't fit vertex buffer?", attrIndex, hint);

            std::visit(Overloaded
            {
                [](std::monostate) {},

                [&result, &attrIndex]
                (models::VertexBuffer::TightlyPackedBuffer const & buffer)
                {
                    std::visit([&result, usage = buffer._usage](auto const & b)
                    {
                        MINIRE_INVARIANT(b, "buffer pointer isn't provided");
                        size_t const vboIndex = reinterpret_cast<size_t>(b->data());
                        opengl::VBO & vbo = result->createVbo(vboIndex, GL_ARRAY_BUFFER);
                        loadBufferData(b, vbo, convertUsage(usage)); // assuming it will bind the VBO
                    },
                    buffer._byteBuffer);
                    assert(result->_vao);
                    result->_vao->enableAttrib(attrIndex);
                    result->_vao->attribPointer(attrIndex, buffer._components,
                                                elementType(buffer),
                                                buffer._normalized ? GL_TRUE : GL_FALSE,
                                                0 /* stride */, buffer._offset);
                },

                [&result, &attrIndex](models::VertexBuffer::StridedBuffer const & buffer)
                {
                    MINIRE_INVARIANT(buffer._byteBuffer, "_byteBuffer pointer isn't provided");
                    size_t const vboIndex = reinterpret_cast<size_t>(buffer._byteBuffer->data());
                    if (opengl::VBO * vbo = result->findVbo(vboIndex); vbo)
                    {
                        vbo->bind();
                    }
                    else
                    {
                        opengl::VBO & newVbo = result->createVbo(vboIndex, GL_ARRAY_BUFFER);
                        newVbo.bufferData(buffer._byteBuffer->size(), buffer._byteBuffer->data(),
                                          convertUsage(buffer._usage)); // assuming it will bind the VBO
                    }
                    result->_vao->enableAttrib(attrIndex);
                    result->_vao->attribPointer(attrIndex, buffer._components,
                                                bufferElementType(buffer._dataType),
                                                buffer._normalized ? GL_TRUE : GL_FALSE,
                                                buffer._stride, buffer._offset);
                },
            }, buffer);
        };

        loadVertexBuffer(vertexBuffer._vertices, locations.vertexAttribute(), "vertices");
        loadVertexBuffer(vertexBuffer._normals, locations.normalAttribute(), "normals");
        loadVertexBuffer(vertexBuffer._tangents, locations.tangentAttribute(), "tangents");
        loadVertexBuffer(vertexBuffer._uvs, locations.uvAttribute(), "uvs");

        result->_aabb = vertexBuffer._aabb;
        result->_doubleSided = vertexBuffer._isDoubleSided;

        return result;
    }
}
