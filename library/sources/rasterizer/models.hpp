#pragma once

#include <minire/content/id.hpp>
#include <minire/utils/aabb.hpp>

#include <rasterizer/model.hpp>
#include <scene/model.hpp>

#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Textures;
    class Ubo;

    class Models
    {
    public:
        explicit Models(Ubo const &,
                        Textures const &,
                        content::Manager &);

        void draw(scene::ModelRef::List &) const;

        void incUse(content::Id const &); // will also load()

        void decUse(content::Id const &); // will also unload()

        utils::Aabb const & aabb(content::Id const &) const;

    private:
        void load(content::Id const &);
        void unload(content::Id const &);

    private:
        using ProgramKey = size_t;
        using Programs = std::unordered_map<ProgramKey,
                                            Model::Program::Sptr>;

        static constexpr ProgramKey kEmptyProgramKey = std::numeric_limits<ProgramKey>::max();

        struct StoreItem
        {
            Model::Uptr _model;
            ProgramKey  _programKey = kEmptyProgramKey; // TODO: move this into Program class
            int         _usage = 0;
            bool        _init = false;
        };

        using Store = std::unordered_map<content::Id, StoreItem>;

        content::Manager & _contentManager;
        Ubo const &        _ubo;
        Textures const &   _textures;
        Store              _store;
        Programs           _programs;
    };
}
