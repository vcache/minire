#pragma once

#include <minire/content/id.hpp>
#include <minire/models/mesh-features.hpp>

#include <memory>

namespace minire::models { struct VertexBuffer; }
namespace minire::opengl { class SharedVertexSubBuffer; }

namespace minire::rasterizer
{
    class Resources;
    class SharedVertexBuffers;

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

        std::shared_ptr<opengl::SharedVertexSubBuffer> build(content::Id const &,
                                                             SharedVertexBuffers &) const;

    private:
        Resources & _resources;
    };
}
