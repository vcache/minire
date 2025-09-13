#pragma once

#include <minire/utils/aabb.hpp>

#include <rasterizer/mesh.hpp>
#include <rasterizer/meshes/id.hpp>

#include <memory>

namespace minire { class Scene; }
namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Materials;
    class MeshToken;
    class Resources;
    class Ubo;

    class Meshes
    {
    public:
        explicit Meshes(Ubo const &,
                        Materials const &,
                        content::Manager &,
                        Resources &);

        void draw(Scene const &) const;

        std::shared_ptr<MeshToken> getMesh(meshes::Id const &);

        std::shared_ptr<MeshToken> getMesh(content::Path const & source,
                                           material::Model::Sptr const & defaultMaterial = {})
        {
            meshes::Id id{._contentPath = source,
                          ._defaultMaterial = defaultMaterial};
            return getMesh(id);
        }

    private:
        std::shared_ptr<MeshToken> load(content::Path const & source,
                                        material::Model::Sptr const & defaultMaterial);

    private:
        content::Manager & _contentManager;
        Resources        & _resources;
        Ubo const &        _ubo;
        Materials const &  _materials;
    };
}
