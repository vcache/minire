#pragma once

#include <minire/content/path.hpp>
#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/utils/aabb.hpp>
#include <minire/utils/std-pair-hash.hpp>

#include <material/types.hpp>
#include <opengl/vertex-buffer.hpp>
#include <rasterizer/materials.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Materials;
    class VertexBuffers;

    class Mesh final
    {
    public:
        using Uptr = std::unique_ptr<Mesh>;

        explicit Mesh(content::Path const & source,
                      Material::Sptr const & defaultMaterial,
                      content::Manager &,
                      Materials const &,
                      VertexBuffers const &);

        utils::Aabb const & aabb() const { return _aabb; }

    public:
        size_t primitives() const { return _primitives.size(); }

        models::MeshFeatures const & meshFeatures(size_t const primitiveIndex) const
        {
            assert(primitiveIndex < _primitives.size());
            return _primitives[primitiveIndex]._meshFeatures;
        }

        opengl::VertexBuffer const & vertexBuffer(size_t primitiveIndex) const
        {
            assert(primitiveIndex < _primitives.size());
            assert(_primitives[primitiveIndex]._buffer);
            return *_primitives[primitiveIndex]._buffer;
        }

        // Primary brushes are Mesh-specific brushes for color pass
        Materials::Brush const & brush(size_t const primitiveIndex) const
        {
            assert(primitiveIndex < _primitives.size());
            assert(_primitives[primitiveIndex]._brush);
            return *_primitives[primitiveIndex]._brush;
        }

        // The caller is resposible to prived the unique key!
        // Should be used carefully, because there is not way to clean up the store.
        // TODO: should be cleaned up somehow
        Materials::Brush::Sptr & extraBrush(size_t primitiveIndex, size_t consumerKey) const
        {
            return _extraBrushes[std::pair(primitiveIndex, consumerKey)];
        }

        static size_t issueConsumerKey();

    private:
        struct Primitive
        {
            // Since the ownership of _buffer is shared, it is valid and sane
            // that some other owners may change the contents of a buffer.
            // For example, it might be used to implement dynamically changable meshes,
            //              or UV-based animations.
            std::shared_ptr<opengl::VertexBuffer> _buffer;
            models::MeshFeatures const            _meshFeatures;
            Materials::Brush::Sptr                _brush;

            explicit Primitive(std::shared_ptr<opengl::VertexBuffer> buffer,
                               models::MeshFeatures const & meshFeatures,
                               Materials::Brush::Sptr const & brush)
                : _buffer(std::move(buffer))
                , _meshFeatures(meshFeatures)
                , _brush(brush)
            {}
        };

        // Just a cache-like store for shadow map instances.
        using ExtraBrushId = std::pair<size_t, size_t>;
        using ExtraBrushes = std::unordered_map<ExtraBrushId, Materials::Brush::Sptr>;

    private:
        void loadPrimitives(content::Path const & source,
                            Material::Sptr const & defaultMaterial,
                            content::Manager & contentManager,
                            Materials const & materials,
                            VertexBuffers const &);

    private:
        std::vector<Primitive> _primitives;
        utils::Aabb            _aabb;
        mutable ExtraBrushes   _extraBrushes;

        friend class Meshes;
    };
}
