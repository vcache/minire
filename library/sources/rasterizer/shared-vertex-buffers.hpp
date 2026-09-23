#pragma once

#include <minire/models/mesh-features.hpp>

#include <opengl/shared-vertex-buffer.hpp>

#include <boost/container_hash/hash.hpp>

#include <memory>
#include <unordered_map>

namespace minire::rasterizer
{
    class Materials;

    class SharedVertexBuffers
    {
    public:
        // Worst-case vertex layout: pos (vec3) + uv (vec2) + normal (vec3) + tangent (vec3) + skin (uvec4 + vec4)
        // (3 + 2 + 3 + 3 + 4 + 4) * sizeof(float) = 76 bytes
        static constexpr size_t kWorstCaseVertexStride = 19 * sizeof(float);

        static size_t elementsForBytes(size_t bytes,
                                       GLenum drawMode = GL_TRIANGLES,
                                       size_t vertexStride = kWorstCaseVertexStride);

        static size_t elementsForBytes(size_t bytes,
                                       models::MeshFeatures const & meshFeatures,
                                       GLenum drawMode = GL_TRIANGLES);

        explicit SharedVertexBuffers(Materials const & materials,
                                     size_t const maxElements);

        opengl::SharedVertexBuffer::Sptr const &
        get(models::MeshFeatures const & meshFeatures,
            GLenum const drawMode = GL_TRIANGLES,
            GLenum const target = GL_STATIC_DRAW);

    private:
        struct Key
        {
            GLenum               _drawMode;
            models::MeshFeatures _meshFeatures;
            GLenum               _target;

            bool operator==(Key const & other) const
            {
                return _drawMode == other._drawMode
                    && _meshFeatures == other._meshFeatures
                    && _target == other._target;
            }
        };

        struct KeyHash
        {
            size_t operator()(Key const & k) const
            {
                size_t result = 0;
                boost::hash_combine(result, k._drawMode);
                boost::hash_combine(result, std::hash<models::MeshFeatures>{}(k._meshFeatures));
                boost::hash_combine(result, k._target);
                return result;
            }
        };

        using Store = std::unordered_map<Key, opengl::SharedVertexBuffer::Sptr, KeyHash>;

        Materials const & _materials;
        size_t const      _maxElements;
        Store             _store;
    };
}
