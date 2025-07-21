#pragma once

#include <minire/content/id.hpp>
#include <minire/utils/aabb.hpp>

#include <rasterizer/mesh.hpp>
#include <scene/model.hpp>

#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Materials;
    class Ubo;

    class Meshes
    {
    public:
        explicit Meshes(Ubo const &,
                        Materials const &,
                        content::Manager &);

        void draw(scene::ModelRef::List &) const;

        void incUse(content::Id const &); // will also load()

        void decUse(content::Id const &); // will also unload()

        utils::Aabb const & aabb(content::Id const &) const;

    private:
        void load(content::Id const &);
        void unload(content::Id const &);

    private:
        struct StoreItem
        {
            Mesh::Uptr _model;
            int        _usage = 0;
            bool       _init = false;
        };

        using Store = std::unordered_map<content::Id, StoreItem>;

        content::Manager & _contentManager;
        Ubo const &        _ubo;
        Materials const &  _materials;
        Store              _store;
    };
}
