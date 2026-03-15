#pragma once

#include <minire/sprite.hpp>

#include <rasterizer/drawable.hpp>

#include <glm/vec2.hpp>

#include <list>
#include <memory>
#include <unordered_map>

namespace minire::rasterizer
{
    class Textures;

    // TODO: it looks very similar to Labels. Maybe some code can be deduplicated
    class Sprites
    {
    public:
        explicit Sprites(Textures const &);

        ~Sprites();

        Sprite::Sptr make(std::string const & name,
                          models::Sprite model);

        Sprite::Sptr const & find(std::string const & name) const;

    public:
        void predraw(Drawable::PtrsList & out) const;

    private:
        class SpriteImpl;
        class Program;

        using SpriteImplSptr = std::shared_ptr<SpriteImpl>;
        using Heap = std::list<SpriteImplSptr>;
        using Index = std::unordered_map<std::string, SpriteImplSptr>;

        Textures const &         _textures;
        std::unique_ptr<Program> _program;
        mutable Heap             _heap;
        mutable Index            _index;
    };
}
