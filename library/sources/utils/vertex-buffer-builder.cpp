#include <utils/vertex-buffer-builder.hpp>

#include <minire/errors.hpp>
#include <minire/models/vertex-buffer.hpp>
#include <minire/utils/always-false.hpp>
#include <minire/utils/overloaded.hpp>

#include <material/types.hpp>
#include <opengl/shared-vertex-buffer.hpp>
#include <opengl/vertex-buffer.hpp>

#include <algorithm>
#include <cassert>
#include <type_traits>

namespace minire::utils
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
            case models::VertexBuffer::DrawMode::kQuads:         return GL_TRIANGLES;
        }
        MINIRE_THROW("unknown draw mode: {}", static_cast<int>(drawMode));
    }

    namespace
    {
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
                    result->_vao.enableAttrib(attrIndex);
                    result->_vao.attribPointer(attrIndex, buffer._components,
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
                    result->_vao.enableAttrib(attrIndex);
                    result->_vao.attribPointer(attrIndex, buffer._components,
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

    namespace
    {
        size_t dataTypeSize(models::VertexBuffer::DataType dt)
        {
            switch(dt)
            {
                case models::VertexBuffer::DataType::kInt8:
                case models::VertexBuffer::DataType::kUInt8:  return 1;
                case models::VertexBuffer::DataType::kInt16:
                case models::VertexBuffer::DataType::kUInt16: return 2;
                case models::VertexBuffer::DataType::kInt32:
                case models::VertexBuffer::DataType::kUInt32:
                case models::VertexBuffer::DataType::kFloat:  return 4;
                case models::VertexBuffer::DataType::kDouble: return 8;
            }
            return 4;
        }

        template<typename T>
        float convertToFloat(T raw, bool normalized)
        {
            if (!normalized)
            {
                return static_cast<float>(raw);
            }
            if constexpr (std::is_same_v<T, int8_t>)
            {
                return std::max(static_cast<float>(raw) / 127.0f, -1.0f);
            }
            else if constexpr (std::is_same_v<T, uint8_t>)
            {
                return static_cast<float>(raw) / 255.0f;
            }
            else if constexpr (std::is_same_v<T, int16_t>)
            {
                return std::max(static_cast<float>(raw) / 32767.0f, -1.0f);
            }
            else if constexpr (std::is_same_v<T, uint16_t>)
            {
                return static_cast<float>(raw) / 65535.0f;
            }
            else if constexpr (std::is_same_v<T, int32_t>)
            {
                return std::max(static_cast<float>(raw) / 2147483647.0f, -1.0f);
            }
            else if constexpr (std::is_same_v<T, uint32_t>)
            {
                return static_cast<float>(raw) / 4294967295.0f;
            }
            else
            {
                return static_cast<float>(raw);
            }
        }

        float getFloatFromBuffer(models::VertexBuffer::Buffer const & buffer,
                                 size_t vertexIndex,
                                 size_t componentIndex)
        {
            return std::visit(Overloaded
            {
                [](std::monostate) -> float { return 0.0f; },
                [vertexIndex, componentIndex](models::VertexBuffer::TightlyPackedBuffer const & buf) -> float
                {
                    return std::visit([&buf, vertexIndex, componentIndex](auto const & b) -> float
                    {
                        using T = typename std::decay_t<decltype(b)>::element_type::value_type;
                        if (!b) return 0.0f;
                        uint8_t const * base = reinterpret_cast<uint8_t const *>(b->data()) + buf._offset;
                        T const * typedBase = reinterpret_cast<T const *>(base);
                        size_t const idx = vertexIndex * buf._components + componentIndex;
                        return convertToFloat(typedBase[idx], buf._normalized);
                    }, buf._byteBuffer);
                },
                [vertexIndex, componentIndex](models::VertexBuffer::StridedBuffer const & buf) -> float
                {
                    if (!buf._byteBuffer) return 0.0f;
                    size_t const elemSize = dataTypeSize(buf._dataType);
                    size_t const stride = buf._stride != 0 ? buf._stride : (buf._components * elemSize);
                    size_t const byteOffset = buf._offset + vertexIndex * stride + componentIndex * elemSize;
                    uint8_t const * ptr = buf._byteBuffer->data() + byteOffset;

                    switch (buf._dataType)
                    {
                        case models::VertexBuffer::DataType::kInt8:
                            return convertToFloat(*reinterpret_cast<int8_t const *>(ptr), buf._normalized);
                        case models::VertexBuffer::DataType::kUInt8:
                            return convertToFloat(*reinterpret_cast<uint8_t const *>(ptr), buf._normalized);
                        case models::VertexBuffer::DataType::kInt16:
                            return convertToFloat(*reinterpret_cast<int16_t const *>(ptr), buf._normalized);
                        case models::VertexBuffer::DataType::kUInt16:
                            return convertToFloat(*reinterpret_cast<uint16_t const *>(ptr), buf._normalized);
                        case models::VertexBuffer::DataType::kInt32:
                            return convertToFloat(*reinterpret_cast<int32_t const *>(ptr), buf._normalized);
                        case models::VertexBuffer::DataType::kUInt32:
                            return convertToFloat(*reinterpret_cast<uint32_t const *>(ptr), buf._normalized);
                        case models::VertexBuffer::DataType::kFloat:
                            return convertToFloat(*reinterpret_cast<float const *>(ptr), buf._normalized);
                        case models::VertexBuffer::DataType::kDouble:
                            return convertToFloat(*reinterpret_cast<double const *>(ptr), buf._normalized);
                    }
                    return 0.0f;
                }
            }, buffer);
        }

        size_t getVertexCount(models::VertexBuffer::Buffer const & vertices)
        {
            return std::visit(Overloaded
            {
                [](std::monostate) -> size_t {
                    MINIRE_THROW("vertices buffer cannot be empty");
                },
                [](models::VertexBuffer::TightlyPackedBuffer const & buf) -> size_t {
                    return std::visit([&buf](auto const & b) -> size_t {
                        MINIRE_INVARIANT(b, "vertices buffer is null");
                        using T = typename std::decay_t<decltype(b)>::element_type::value_type;
                        size_t totalBytes = b->size() * sizeof(T);
                        if (buf._offset >= totalBytes) return 0;
                        size_t usableBytes = totalBytes - buf._offset;
                        size_t elementCount = usableBytes / sizeof(T);
                        size_t comps = buf._components > 0 ? buf._components : 3;
                        return elementCount / comps;
                    }, buf._byteBuffer);
                },
                [](models::VertexBuffer::StridedBuffer const & buf) -> size_t {
                    MINIRE_INVARIANT(buf._byteBuffer, "vertices strided buffer is null");
                    size_t totalBytes = buf._byteBuffer->size();
                    if (buf._offset >= totalBytes) return 0;
                    size_t usableBytes = totalBytes - buf._offset;
                    size_t comps = buf._components > 0 ? buf._components : 3;
                    size_t elemSize = dataTypeSize(buf._dataType);
                    size_t stride = buf._stride != 0 ? buf._stride : (comps * elemSize);
                    return stride > 0 ? (usableBytes / stride) : 0;
                }
            }, vertices);
        }

        std::vector<uint32_t> getIndices(models::VertexBuffer::ElementBuffer const & elements, size_t vertexCount)
        {
            return std::visit(Overloaded
            {
                [vertexCount](std::monostate) -> std::vector<uint32_t> {
                    std::vector<uint32_t> res(vertexCount);
                    for (size_t i = 0; i < vertexCount; ++i) res[i] = static_cast<uint32_t>(i);
                    return res;
                },
                [](models::VertexBuffer::BufferSptr<uint8_t> const & b) -> std::vector<uint32_t> {
                    MINIRE_INVARIANT(b, "indices buffer is null");
                    std::vector<uint32_t> res(b->size());
                    for (size_t i = 0; i < b->size(); ++i) res[i] = (*b)[i];
                    return res;
                },
                [](models::VertexBuffer::BufferSptr<uint16_t> const & b) -> std::vector<uint32_t> {
                    MINIRE_INVARIANT(b, "indices buffer is null");
                    std::vector<uint32_t> res(b->size());
                    for (size_t i = 0; i < b->size(); ++i) res[i] = (*b)[i];
                    return res;
                },
                [](models::VertexBuffer::BufferSptr<uint32_t> const & b) -> std::vector<uint32_t> {
                    MINIRE_INVARIANT(b, "indices buffer is null");
                    return *b;
                }
            }, elements._indecesBuffer);
        }

        struct ConvertedBuffers
        {
            std::vector<uint32_t> _indices;
            std::vector<float>    _vertices;
            utils::Aabb           _aabb;
        };

        ConvertedBuffers convertVertexBuffer(models::VertexBuffer const & vertexBuffer,
                                             opengl::SharedVertexBuffer const & sharedVertexBuffer)
        {
            size_t const vertexCount = getVertexCount(vertexBuffer._vertices);
            std::vector<uint32_t> rawIndices = getIndices(vertexBuffer._elements, vertexCount);

            std::vector<uint32_t> indices;
            if (vertexBuffer._drawMode == models::VertexBuffer::DrawMode::kQuads)
            {
                MINIRE_INVARIANT(rawIndices.size() % 4 == 0,
                                 "quad indices count must be divisible by 4, got {}", rawIndices.size());
                indices.reserve(rawIndices.size() / 4 * 6);
                for (size_t i = 0; i < rawIndices.size(); i += 4)
                {
                    indices.push_back(rawIndices[i + 0]);
                    indices.push_back(rawIndices[i + 1]);
                    indices.push_back(rawIndices[i + 2]);
                    indices.push_back(rawIndices[i + 0]);
                    indices.push_back(rawIndices[i + 2]);
                    indices.push_back(rawIndices[i + 3]);
                }
            }
            else
            {
                indices = std::move(rawIndices);
            }

            models::MeshFeatures const & mf = sharedVertexBuffer.meshFeatures();
            size_t const floatStride = sharedVertexBuffer.stride();
            std::vector<float> vertexData(vertexCount * floatStride);

            utils::Aabb computedAabb;

            for (size_t i = 0; i < vertexCount; ++i)
            {
                float * vertexPtr = vertexData.data() + i * floatStride;

                float const x = getFloatFromBuffer(vertexBuffer._vertices, i, 0);
                float const y = getFloatFromBuffer(vertexBuffer._vertices, i, 1);
                float const z = getFloatFromBuffer(vertexBuffer._vertices, i, 2);

                vertexPtr[0] = x;
                vertexPtr[1] = y;
                vertexPtr[2] = z;

                glm::vec3 const pos(x, y, z);
                if (i == 0)
                {
                    computedAabb = utils::Aabb(pos, pos);
                }
                else
                {
                    computedAabb.extend(pos);
                }

                size_t offset = 3;

                if (mf.hasUv())
                {
                    vertexPtr[offset + 0] = getFloatFromBuffer(vertexBuffer._uvs, i, 0);
                    vertexPtr[offset + 1] = getFloatFromBuffer(vertexBuffer._uvs, i, 1);
                    offset += 2;
                }

                if (mf.hasNormal())
                {
                    vertexPtr[offset + 0] = getFloatFromBuffer(vertexBuffer._normals, i, 0);
                    vertexPtr[offset + 1] = getFloatFromBuffer(vertexBuffer._normals, i, 1);
                    vertexPtr[offset + 2] = getFloatFromBuffer(vertexBuffer._normals, i, 2);
                    offset += 3;
                }

                if (mf.hasTangent())
                {
                    vertexPtr[offset + 0] = getFloatFromBuffer(vertexBuffer._tangents, i, 0);
                    vertexPtr[offset + 1] = getFloatFromBuffer(vertexBuffer._tangents, i, 1);
                    vertexPtr[offset + 2] = getFloatFromBuffer(vertexBuffer._tangents, i, 2);
                    offset += 3;
                }

                assert(offset == floatStride);
            }

            utils::Aabb aabb = !vertexBuffer._aabb.empty() ? vertexBuffer._aabb : computedAabb;

            return ConvertedBuffers
            {
                ._indices = std::move(indices),
                ._vertices = std::move(vertexData),
                ._aabb = aabb,
            };
        }
    }

    std::unique_ptr<opengl::SharedVertexSubBuffer>
    createVertexSubBuffer(models::VertexBuffer const & vertexBuffer,
                          opengl::SharedVertexBuffer & sharedVertexBuffer)
    {
        auto converted = convertVertexBuffer(vertexBuffer, sharedVertexBuffer);

        return sharedVertexBuffer.create(
            converted._indices.size(),
            converted._aabb,
            converted._indices.size() * sizeof(uint32_t),
            converted._indices.data(),
            converted._vertices.size() * sizeof(float),
            converted._vertices.data());
    }

    void
    updateVertexSubBuffer(models::VertexBuffer const & vertexBuffer,
                          opengl::SharedVertexSubBuffer & subBuffer)
    {
        auto svb = subBuffer.sharedVertexBuffer();
        MINIRE_INVARIANT(svb, "SharedVertexBuffer has expired");

        auto converted = convertVertexBuffer(vertexBuffer, *svb);

        svb->update(
            subBuffer,
            converted._indices.size(),
            converted._aabb,
            converted._indices.size() * sizeof(uint32_t),
            converted._indices.data(),
            converted._vertices.size() * sizeof(float),
            converted._vertices.data());
    }
}
