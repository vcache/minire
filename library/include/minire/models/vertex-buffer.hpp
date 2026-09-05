#pragma once

#include <minire/utils/aabb.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace minire::models
{
    /**
     * A OpenGL-compatible representation of a mesh.
     * Usually, minire::models::Mesh should be used instead.
     * This model is intended for the implementation of dynamically
     * generated meshes such as landscapes, random or unique meshes and so on.
     * */
    struct VertexBuffer
    {
        enum class DrawMode
        {
            kLines,         // GL_LINES
            kLineStrip,     // GL_LINE_STRIP
            kLineLoop,      // GL_LINE_LOOP
            kTriangles,     // GL_TRIANGLES
            kTriangleStrip, // GL_TRIANGLE_STRIP
            kTriangleFan,   // GL_TRIANGLE_FAN
            kQuads,         // GL_QUADS
        };

        enum class DataType
        {
            kInt8,          // GL_BYTE
            kUInt8,         // GL_UNSIGNED_BYTE
            kInt16,         // GL_SHORT
            kUInt16,        // GL_UNSIGNED_SHORT
            kInt32,         // GL_INT
            kUInt32,        // GL_UNSIGNED_INT
            kFloat,         // GL_FLOAT
            kDouble,        // GL_DOUBLE
        };

        // NOTE: Typically Usage::k*Draw are usable, but all variants are enumerated here
        //       for completeness and future extensions.
        enum class Usage
        {
            kStreamDraw,    // GL_STREAM_DRAW
            kStreamRead,    // GL_STREAM_READ
            kStreamCopy,    // GL_STREAM_COPY
            kStaticDraw,    // GL_STATIC_DRAW
            kStaticRead,    // GL_STATIC_READ
            kStaticCopy,    // GL_STATIC_COPY
            kDynamicDraw,   // GL_DYNAMIC_DRAW
            kDynamicRead,   // GL_DYNAMIC_READ
            kDynamicCopy,   // GL_DYNAMIC_COPY
        };

        template<typename T>
        using BufferSptr = std::shared_ptr<std::vector<T>>;

        struct CommonBufferParams
        {
            // Specifies the number of components per generic vertex attribute.
            // Must be 1, 2, 3, 4.
            size_t _components = 4;

            // Specifies whether fixed-point data values should be normalized (GL_TRUE) or
            // converted directly as fixed-point values (GL_FALSE) when they are accessed.
            bool   _normalized = false;

            // Specifies a offset of the first component of the first generic vertex attribute
            // in the array in the data store of the buffer
            size_t _offset = 0;

            // Specifies the expected usage pattern of the data store.
            Usage  _usage = Usage::kStaticDraw;
        };

        // See specification of glVertexAttribPointer for details. Some excerpts from
        // the documentation are posted here for convenience.
        // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glVertexAttribPointer.xhtml
        struct TightlyPackedBuffer
            : public CommonBufferParams
        {
            using ByteBuffer = std::variant<BufferSptr<int8_t>,     // GL_BYTE
                                            BufferSptr<uint8_t>,    // GL_UNSIGNED_BYTE
                                            BufferSptr<int16_t>,    // GL_SHORT
                                            BufferSptr<uint16_t>,   // GL_UNSIGNED_SHORT
                                            BufferSptr<int32_t>,    // GL_INT
                                            BufferSptr<uint32_t>,   // GL_UNSIGNED_INT
                                            BufferSptr<float>,      // GL_FLOAT
                                            BufferSptr<double>>;    // GL_DOUBLE

            ByteBuffer _byteBuffer;
        };

        struct StridedBuffer
            : public CommonBufferParams
        {
            using Buffer = BufferSptr<uint8_t>;

            Buffer   _byteBuffer;

            // Specifies the byte offset between consecutive generic vertex attributes.
            // If stride is 0, the generic vertex attributes are understood to be tightly
            // packed in the array.
            size_t   _stride = 0;

            // Specifies the data type of each component in the array.
            DataType _dataType = DataType::kFloat;
        };

        struct ElementBuffer
        {
            using IndecesBuffer = std::variant<std::monostate,          // for "draw array"
                                               BufferSptr<uint8_t>,     // GL_UNSIGNED_BYTE
                                               BufferSptr<uint16_t>,    // GL_UNSIGNED_SHORT
                                               BufferSptr<uint32_t>>;   // GL_UNSIGNED_INT

            IndecesBuffer _indecesBuffer;
            Usage         _usage = Usage::kStaticDraw;
        };

        using Buffer = std::variant<std::monostate,
                                    TightlyPackedBuffer,
                                    StridedBuffer>;

        DrawMode      _drawMode;
        ElementBuffer _elements;
        Buffer        _vertices;
        Buffer        _normals;
        Buffer        _tangents;
        Buffer        _uvs;
        utils::Aabb   _aabb;    // TODO: make it optional
        bool          _isDoubleSided;
    };
}
