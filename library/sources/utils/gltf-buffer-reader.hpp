#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#include <minire/errors.hpp>
#include <minire/utils/always-false.hpp>
#include <minire/utils/demangle.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <tinygltf/tiny_gltf.h>

namespace minire::utils
{

    // TODO: this code must be very buggy,
    //       should cover it with tests

    template<typename T>
    struct StaticCast
    {
        template<typename Src>
        T operator()(Src src) const { return static_cast<T>(src); }
    };

    template<typename T>
    struct DenormCast;

    template<>
    struct DenormCast<float>
    {
        float operator()(int8_t c) const
        {
            return std::max<float>(static_cast<float>(c) / 127.0f, -1.0f);
        }

        float operator()(uint8_t c) const
        {
            return static_cast<float>(c) / 255.0f;
        }

        float operator()(int16_t c) const
        {
            return std::max<float>(static_cast<float>(c) / 32767.0f, -1.0f);
        }

        float operator()(uint16_t c) const
        {
            return static_cast<float>(c) / 65535.0f;
        }
    };

    // TODO: there is a duplicated code in readElements

    template<typename SrcType, bool kIsNormalized, typename DstType,
             typename = std::enable_if_t<std::is_arithmetic_v<DstType>>>
    void readElements(::tinygltf::Accessor const & accessor,
                      ::tinygltf::BufferView const & bufferView,
                      ::tinygltf::Buffer const & buffer,
                      std::vector<DstType> & output)
    {
        assert(sizeof(SrcType) == ::tinygltf::GetComponentSizeInBytes(accessor.componentType));

        bool isTightlyPacked = false;
        if constexpr (std::is_same_v<SrcType, DstType>)
        {
            isTightlyPacked = bufferView.byteStride == 0;
        }

        if (isTightlyPacked)
        {
            uint8_t const * byteBegin = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
            uint8_t const * byteEnd = buffer.data.data() + bufferView.byteOffset + bufferView.byteLength;
            DstType const * floatBegin = reinterpret_cast<DstType const *>(byteBegin);
            DstType const * floatEnd = floatBegin + accessor.count;
            MINIRE_INVARIANT(reinterpret_cast<uint8_t const*>(floatEnd) <= byteEnd,
                             "tightly packed buffer overflow");
            output.reserve(accessor.count);
            output.assign(floatBegin, floatEnd);
        }
        else
        {
            using Converter = std::conditional_t<kIsNormalized, DenormCast<DstType>,
                                                                StaticCast<DstType>>;
            Converter converter{};

            uint8_t const * byteBuffer = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
            uint8_t const * byteBufferEnd = buffer.data.data() + bufferView.byteOffset + bufferView.byteLength;
            output.resize(accessor.count);

            size_t const byteStride = bufferView.byteStride > 0 ? bufferView.byteStride
                                                                : sizeof(SrcType);
            MINIRE_INVARIANT(byteStride >= sizeof(SrcType), "too short stride ({}) for {}",
                             byteStride, utils::demangle<SrcType>());

            for(size_t i = 0; i < accessor.count; ++i)
            {
                MINIRE_INVARIANT(byteBuffer < byteBufferEnd, "buffer overflow: i={} at {}/{}/{}",
                                 i, accessor.name, bufferView.name, buffer.uri);

                SrcType const * srcPtr = reinterpret_cast<SrcType const *>(byteBuffer);
                output[i] = converter(*srcPtr);
                byteBuffer += byteStride;
            }
        }
    }

    template<typename SrcType, bool kIsNormalized>
    void readElements(::tinygltf::Accessor const & accessor,
                      ::tinygltf::BufferView const & bufferView,
                      ::tinygltf::Buffer const & buffer,
                      std::vector<glm::vec3> & output)
    {
        assert(sizeof(SrcType) == ::tinygltf::GetComponentSizeInBytes(accessor.componentType));

        using DstType = glm::vec3::value_type;
        using Converter = std::conditional_t<kIsNormalized, DenormCast<DstType>,
                                                            StaticCast<DstType>>;
        Converter converter{};

        uint8_t const * byteBuffer = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        uint8_t const * byteBufferEnd = buffer.data.data() + bufferView.byteOffset + bufferView.byteLength;
        output.resize(accessor.count);

        size_t const byteStride = bufferView.byteStride > 0 ? bufferView.byteStride
                                                            : sizeof(SrcType) * 3;
        MINIRE_INVARIANT(byteStride >= sizeof(SrcType) * 3,
                         "too short stride ({}) for VEC3 of {}", byteStride,
                         utils::demangle<SrcType>());

        for(size_t i = 0; i < accessor.count; ++i)
        {
            MINIRE_INVARIANT(byteBuffer < byteBufferEnd, "buffer overflow: i={} at {}/{}/{}",
                             i, accessor.name, bufferView.name, buffer.uri);

            SrcType const * srcPtr = reinterpret_cast<SrcType const *>(byteBuffer);
            output[i] = glm::vec3(converter(srcPtr[0]),
                                  converter(srcPtr[1]),
                                  converter(srcPtr[2]));
            byteBuffer += byteStride;
        }
    }

    template<typename SrcType, bool kIsNormalized>
    void readElements(::tinygltf::Accessor const & accessor,
                      ::tinygltf::BufferView const & bufferView,
                      ::tinygltf::Buffer const & buffer,
                      std::vector<glm::quat> & output)
    {
        assert(sizeof(SrcType) == ::tinygltf::GetComponentSizeInBytes(accessor.componentType));

        using DstType = glm::quat::value_type;
        using Converter = std::conditional_t<kIsNormalized, DenormCast<DstType>,
                                                            StaticCast<DstType>>;
        Converter converter{};

        uint8_t const * byteBuffer = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        uint8_t const * byteBufferEnd = buffer.data.data() + bufferView.byteOffset + bufferView.byteLength;
        output.resize(accessor.count);

        size_t const byteStride = bufferView.byteStride > 0 ? bufferView.byteStride
                                                            : sizeof(SrcType) * 4;
        MINIRE_INVARIANT(byteStride >= sizeof(SrcType) * 4,
                         "too short stride ({}) for VEC4 of {}", byteStride,
                         utils::demangle<SrcType>());

        for(size_t i = 0; i < accessor.count; ++i)
        {
            MINIRE_INVARIANT(byteBuffer < byteBufferEnd, "buffer overflow: i={} at {}/{}/{}",
                             i, accessor.name, bufferView.name, buffer.uri);

            SrcType const * srcPtr = reinterpret_cast<SrcType const *>(byteBuffer);
            output[i] = glm::quat(converter(srcPtr[3]), // w
                                  converter(srcPtr[0]), // x
                                  converter(srcPtr[1]), // y
                                  converter(srcPtr[2]));// z

            byteBuffer += byteStride;
        }
    }

    template<typename SrcType, bool kIsNormalized>
    void readElements(::tinygltf::Accessor const & accessor,
                      ::tinygltf::BufferView const & bufferView,
                      ::tinygltf::Buffer const & buffer,
                      std::vector<glm::mat4> & output)
    {
        assert(sizeof(SrcType) == ::tinygltf::GetComponentSizeInBytes(accessor.componentType));

        static_assert(std::is_same_v<SrcType, float>,
                      "only FLOAT component type is allowed for MAT4");
        static_assert(!kIsNormalized, "MAT4 cannot be normalized");

        uint8_t const * byteBuffer = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        uint8_t const * byteBufferEnd = buffer.data.data() + bufferView.byteOffset + bufferView.byteLength;
        output.resize(accessor.count);

        size_t const byteStride = bufferView.byteStride > 0 ? bufferView.byteStride
                                                            : sizeof(SrcType) * 16;
        MINIRE_INVARIANT(byteStride >= sizeof(SrcType) * 16,
                         "too short stride ({}) for MAT4 of {}", byteStride,
                         utils::demangle<SrcType>());

        for(size_t i = 0; i < accessor.count; ++i)
        {
            MINIRE_INVARIANT(byteBuffer < byteBufferEnd, "buffer overflow: i={} at {}/{}/{}",
                             i, accessor.name, bufferView.name, buffer.uri);

            SrcType const * srcPtr = reinterpret_cast<SrcType const *>(byteBuffer);
            output[i] = glm::make_mat4x4(srcPtr); // a column-major order is in both glTF and GLM
            byteBuffer += byteStride;
        }

        assert(byteBuffer == byteBufferEnd);
    }

    template<typename T>
    constexpr int expectedAccessType()
    {
        if constexpr(std::is_same_v<T, float>)
        {
            return TINYGLTF_TYPE_SCALAR;
        }
        else if constexpr(std::is_same_v<T, glm::vec3>)
        {
            return TINYGLTF_TYPE_VEC3;
        }
        else if constexpr(std::is_same_v<T, glm::quat>)
        {
            return TINYGLTF_TYPE_VEC4;
        }
        else if constexpr(std::is_same_v<T, glm::mat4>)
        {
            return TINYGLTF_TYPE_MAT4;
        }
        else
        {
            static_assert(utils::kAlwaysFalse<T>::value,
                          "no reader for a requested type");
        }
    }

    template<typename T>
    void readData(::tinygltf::Accessor const & accessor,
                  ::tinygltf::BufferView const & bufferView,
                  ::tinygltf::Buffer const & buffer,
                  std::vector<T> & output)
    {
        MINIRE_INVARIANT(accessor.type == expectedAccessType<T>(), "unexpected accessor type ({}) for {}",
                         accessor.type, utils::demangle<T>());

        if constexpr(std::is_same_v<T, float> ||
                     std::is_same_v<T, glm::vec3> ||
                     std::is_same_v<T, glm::quat>)
        {
            switch(accessor.componentType)
            {
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                    return accessor.normalized ? readElements<int8_t, true>(accessor, bufferView, buffer, output)
                                               : readElements<int8_t, false>(accessor, bufferView, buffer, output);

                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    return accessor.normalized ? readElements<uint8_t, true>(accessor, bufferView, buffer, output)
                                               : readElements<uint8_t, false>(accessor, bufferView, buffer, output);

                case TINYGLTF_COMPONENT_TYPE_SHORT:
                    return accessor.normalized ? readElements<int16_t, true>(accessor, bufferView, buffer, output)
                                               : readElements<int16_t, false>(accessor, bufferView, buffer, output);

                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    return accessor.normalized ? readElements<uint16_t, true>(accessor, bufferView, buffer, output)
                                               : readElements<uint16_t, false>(accessor, bufferView, buffer, output);

                case TINYGLTF_COMPONENT_TYPE_INT:
                    MINIRE_THROW("INT accessor component type isn't implemented for animation");

                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    MINIRE_INVARIANT(!accessor.normalized,
                                     "FLOAT accessor component type cannot be normalized (see 5.1.4)");
                    readElements<float, false>(accessor, bufferView, buffer, output);
                    break;

                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                    MINIRE_THROW("DOUBLE accessor component type isn't implemented for animation");

                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    MINIRE_THROW("UNSIGNED_INT accessor component type cannot "
                                 "be used for animation (see 5.1.3)");

                default:
                    MINIRE_THROW("unknown animation accessor component type: {}, {}",
                                 accessor.componentType, accessor.name);
            }
        }
        else if constexpr(std::is_same_v<T, glm::mat4>)
        {
            MINIRE_INVARIANT(TINYGLTF_COMPONENT_TYPE_FLOAT == accessor.componentType,
                             "FLOAT component type expected for {} accessor, but got {}",
                             utils::demangle<T>(), accessor.componentType);
            MINIRE_INVARIANT(!accessor.normalized,
                             "FLOAT accessor component type cannot be normalized (see 5.1.4)");
            readElements<float, false>(accessor, bufferView, buffer, output);
        }
        else
        {
            static_assert(utils::kAlwaysFalse<T>::value,
                          "no reader for a requested type");
        }
    }

    template<typename T>
    std::shared_ptr<std::vector<T>> readAccessor(int rawAccessorIndex,
                                                 ::tinygltf::Model const & model)
    {
        // fetch an Accessor
        MINIRE_INVARIANT(rawAccessorIndex >= 0, "bad sampler index: {}", rawAccessorIndex);
        size_t const accessorIndex = static_cast<size_t>(rawAccessorIndex);
        MINIRE_INVARIANT(accessorIndex < model.accessors.size(), "bad sampler: {} >= {}",
                         accessorIndex, model.accessors.size());
        ::tinygltf::Accessor const & accessor = model.accessors[accessorIndex];

        MINIRE_INVARIANT(!accessor.sparse.isSparse, "sparse accessors isn't supported: {}", accessor.name);

        // fetch an BufferView
        MINIRE_INVARIANT(accessor.bufferView >= 0, "bad bufferView: {}, {}", accessor.bufferView, accessor.name);
        size_t const bufferViewIndex = static_cast<size_t>(accessor.bufferView);
        MINIRE_INVARIANT(bufferViewIndex < model.bufferViews.size(), "bad bufferView: {} >= {}, {}",
                         bufferViewIndex, model.bufferViews.size(), accessor.name);
        ::tinygltf::BufferView const & bufferView = model.bufferViews[bufferViewIndex];

        MINIRE_INVARIANT(bufferView.target == 0,
                         "unexpected BufferView target ({}) of animation channel accessor: {}",
                         bufferView.target, bufferView.name);

        // fetch an Buffer
        MINIRE_INVARIANT(bufferView.buffer >= 0, "bad buffer: {}, {}", bufferView.buffer, bufferView.name);
        size_t const bufferIndex = static_cast<size_t>(bufferView.buffer);
        MINIRE_INVARIANT(bufferIndex < model.buffers.size(), "bad buffer: {} >= {}, {}",
                         bufferIndex, model.buffers.size(), bufferView.name);
        ::tinygltf::Buffer const & buffer = model.buffers[bufferIndex];

        // create resulting store
        std::shared_ptr<std::vector<T>> result = std::make_shared<std::vector<T>>();
        readData(accessor, bufferView, buffer, *result);

        return result;
    }
}
