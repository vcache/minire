#pragma once

#include <minire/content/path.hpp>
#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/utils/aabb.hpp>

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

        void draw(glm::mat4 const & transform,
                  glm::vec3 const & ambientLight,
                  glm::vec3 const & emissiveFactor,
                  material::TextureRefs const & directionalLightsShadowMaps,
                  material::TextureRefs const & pointLightsShadowMaps,
                  material::SkinningVector const & skinningVector,
                  uint32_t const meshId) const;

        // Position attrib is guaranteed to be at index 0.
        void drawBare() const;

        utils::Aabb const & aabb() const { return _aabb; }

    public:
        using PrimitiveTraits = std::tuple<models::MeshFeatures const &,
                                           material::Locations const &>;

        size_t primitives() const { return _primitives.size(); }

        PrimitiveTraits primitiveTraits(size_t const primitiveIndex) const;

        void drawBare(size_t const primitiveIndex) const;

        // The caller is resposible to prived the unique key!
        // Should be used carefully, because there is not way to clean up the store.
        // TODO: should be cleaned up somehow
        Materials::Brush::Sptr & extraBrush(size_t consumerKey) const
        {
            return _extraBrushes[consumerKey];
        }

        static size_t issueConsumerKey();

    private:
        struct MaterialData
        {
            Materials::Brush::Sptr _brush;
            std::vector<size_t>    _primitives;
        };

        struct Primitive
        {
            // Since the ownership of _buffer is shared, it is valid and sane
            // that some other owners may change the contents of a buffer.
            // For example, it might be used to implement dynamically changable meshes,
            //              or UV-based animations.
            //
            // (!) Despite that, other owners  MUST  guarantee, that mesh alternations
            // will be done in a compatible with an initially given attrib locations way!
            std::shared_ptr<opengl::VertexBuffer> _buffer;
            models::MeshFeatures const            _meshFeatures;
            material::Locations const             _attribLocations;

            explicit Primitive(std::shared_ptr<opengl::VertexBuffer> buffer,
                               models::MeshFeatures const & meshFeatures,
                               material::Locations const & attribLocations)
                : _buffer(std::move(buffer))
                , _meshFeatures(meshFeatures)
                , _attribLocations(attribLocations)
            {}
        };

        // Just a cache-like store for shadow map instances.
        using ExtraBrushes = std::unordered_map<size_t, Materials::Brush::Sptr>;

    private:
        void loadPrimitives(content::Path const & source,
                            Material::Sptr const & defaultMaterial,
                            content::Manager & contentManager,
                            Materials const & materials,
                            VertexBuffers const &);

    private:
        std::vector<MaterialData> _materials;
        std::vector<Primitive>    _primitives;
        utils::Aabb               _aabb;
        mutable ExtraBrushes      _extraBrushes;

        friend class Meshes;
    };
}
