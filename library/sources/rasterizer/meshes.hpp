#pragma once

#include <minire/material.hpp>
#include <minire/utils/aabb.hpp>

#include <rasterizer/culled-objects.hpp>
#include <rasterizer/mesh.hpp>
#include <rasterizer/meshes/id.hpp>

#include <memory>

namespace minire { class SceneImpl; }
namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Materials;
    class Resources;
    class VertexBuffers;

    class Meshes
    {
    public:
        explicit Meshes(Materials const &,
                        VertexBuffers const &,
                        content::Manager &,
                        Resources &);

        void draw(SceneImpl const &,
                  CulledPrimitives const &,
                  material::TextureRefs const & directionalLightsShadowMaps,
                  material::TextureRefs const & pointLightsShadowMaps) const;

        std::shared_ptr<Mesh> getMesh(meshes::Id const &);

        std::shared_ptr<Mesh> getMesh(content::Handle const & source,
                                      Material::Sptr const & defaultMaterial = {})
        {
            return getMesh(meshes::Id(source, defaultMaterial));
        }

    private:
        std::shared_ptr<Mesh> load(content::Handle const & source,
                                   Material::Sptr const & defaultMaterial);

    private:
        content::Manager &    _contentManager;
        Resources &           _resources;
        Materials const &     _materials;
        VertexBuffers const & _vertexBuffers;
    };
}
