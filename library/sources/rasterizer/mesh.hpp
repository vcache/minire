#pragma once

#include <minire/content/path.hpp>
#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/utils/aabb.hpp>

#include <opengl/vertex-buffer.hpp>

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
    class Ubo;
    class VertexBuffers;

    class Mesh final
    {
    public:
        using Uptr = std::unique_ptr<Mesh>;

        explicit Mesh(content::Path const & source,
                      material::Model::Sptr const & defaultMaterial,
                      content::Manager &,
                      Materials const &,
                      Ubo const &,
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
                                           material::Program::Locations const &>;

        size_t primitives() const { return _primitives.size(); }

        PrimitiveTraits primitiveTraits(size_t const primitiveIndex) const;

        void drawBare(size_t const primitiveIndex) const;

    private:
        struct Material
        {
            material::Program::Sptr  _matProgram;
            material::Instance::Uptr _matInstance;
            std::vector<size_t>      _primitives;
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
            material::Program::Locations const    _attribLocations;

            explicit Primitive(std::shared_ptr<opengl::VertexBuffer> buffer,
                               models::MeshFeatures const & meshFeatures,
                               material::Program::Locations const & attribLocations)
                : _buffer(std::move(buffer))
                , _meshFeatures(meshFeatures)
                , _attribLocations(attribLocations)
            {}
        };

    private:
        void loadPrimitives(content::Path const & source,
                            material::Model::Sptr const & defaultMaterial,
                            content::Manager & contentManager,
                            Materials const & materials,
                            Ubo const & ubo,
                            VertexBuffers const &);

    private:
        std::vector<Material>  _materials;
        std::vector<Primitive> _primitives;
        utils::Aabb            _aabb;

        friend class Meshes;
    };
}
