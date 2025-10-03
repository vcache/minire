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

        // assuming that caller will "use" gl's program!
        void draw(glm::mat4 const &,
                  glm::vec3 const & ambientLight,
                  glm::vec3 const & emissiveFactor) const;

        utils::Aabb const & aabb() const { return _aabb; }

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

            explicit Primitive(std::shared_ptr<opengl::VertexBuffer> buffer)
                : _buffer(std::move(buffer))
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
