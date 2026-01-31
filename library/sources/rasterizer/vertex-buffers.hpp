#pragma once

#include <minire/content/id.hpp>
#include <minire/material.hpp> // TODO: included only for Program::Locations
#include <minire/models/mesh-features.hpp>

#include <memory>

namespace minire::models { struct VertexBuffer; }
namespace minire::opengl { struct VertexBuffer; }

namespace minire::rasterizer
{
    class Resources;

    // TODO: support skinning for vertex buffers
    class VertexBuffers
    {
    public:
        explicit VertexBuffers(Resources &);

        void create(content::Id const & id,
                    models::VertexBuffer const & vertexBuffer,
                    bool const override);

        void dispose(content::Id const & id);

        models::MeshFeatures meshFeatures(content::Id const &) const;

        std::shared_ptr<opengl::VertexBuffer> build(content::Id const &,
                                                    material::Program::Locations const &) const;

    private:
        Resources & _resources;
    };
}
