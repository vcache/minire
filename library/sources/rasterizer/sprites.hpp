#pragma once

#include <minire/content/id.hpp>
#include <minire/utils/rect.hpp>

#include <rasterizer/drawable.hpp>

#include <glm/vec2.hpp>

#include <string>
#include <memory>
#include <unordered_map>

namespace minire::rasterizer
{
    class Textures;

    class Sprites
    {
        class Sprite;
        class Program;

    public:
        explicit Sprites(Textures const &);

        ~Sprites(); // because of std::unique_ptr<Program>

        void create(std::string const & id,
                    content::Id const & texture,
                    utils::Patch const & patch, // in pixels, on a texture
                    glm::vec2 const & position,
                    glm::vec2 const & dimensions,
                    bool const visible,
                    size_t const zOrder);

        void move(std::string const & id,
                  glm::vec2 const & position);

        void resize(std::string const & id,
                    glm::vec2 const & dimensions);

        void setArea(std::string const & id,
                     glm::vec2 const & position,
                     glm::vec2 const & dimensions);

        void visible(std::string const & id,
                     bool visible);

        void setZOrder(std::string const & id,
                       size_t zOrder);

        void remove(std::string const & id);

    public:
        void predraw(Drawable::PtrsList & out) const;

    private:
        Sprite & find(std::string const &) const;

    private:
        using SpritePtr = std::unique_ptr<Sprite>;
        using Store = std::unordered_map<std::string, SpritePtr>;

        Textures const &          _textures;
        std::unique_ptr<Program>  _program;
        Store                     _store;
    };
}
