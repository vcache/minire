#pragma once

#include <minire/content/path.hpp>
#include <minire/material.hpp>
#include <minire/utils/aabb.hpp>
#include <minire/utils/std-pair-hash.hpp>

#include <rasterizer/mesh.hpp>

#include <memory>
#include <unordered_map>

namespace minire { class Scene; }
namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Materials;
    class Ubo;
    class MeshToken;

    class Meshes
    {
    public:
        explicit Meshes(Ubo const &,
                        Materials const &,
                        content::Manager &);

        void draw(Scene const &) const;

        std::shared_ptr<MeshToken> getMesh(content::Path const & source,
                                           material::Model::Sptr const & defaultMaterial = {});

    private:
        std::shared_ptr<MeshToken> load(content::Path const & source,
                                        material::Model::Sptr const & defaultMaterial);

    private:
        using Key = std::pair<content::Path, material::Model::Sptr>;
        using Store = std::unordered_map<Key, std::weak_ptr<MeshToken>>;

        content::Manager & _contentManager;
        Ubo const &        _ubo;
        Materials const &  _materials;
        Store              _store;
    };
}
