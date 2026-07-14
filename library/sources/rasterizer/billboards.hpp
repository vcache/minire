#pragma once

#include <minire/models/billboard.hpp>

namespace minire { class SceneImpl; }
namespace minire::content { class Manager; }
namespace minire::scene { class Viewpoint; }
namespace minire::utils { class Aabb; }
namespace minire::utils { class FrustumPlanes; }

#include <glm/mat4x4.hpp>

#include <memory>

namespace minire::rasterizer
{
    class Billboard;
    class Fonts;
    class Textures;

    class Billboards
    {
    public:
        explicit Billboards(content::Manager &,
                            Fonts const &,
                            Textures const &);

        std::shared_ptr<Billboard> create(models::Billboard const &) const;

        void draw(SceneImpl const &) const;

        utils::Aabb const & aabb(Billboard const &) const;

    public:
        class Program;
        using ProgramSptr = std::shared_ptr<Program>;

    private:
        std::shared_ptr<Billboard> create(models::Billboard const &,
                                          models::Billboard::Sprite const &) const;

        std::shared_ptr<Billboard> create(models::Billboard const &,
                                          models::Billboard::Label const &) const;

    private:
        content::Manager & _contentManager;
        Fonts const &      _fonts;
        Textures const &   _textures;
        ProgramSptr        _worldPlacedSprite;
        ProgramSptr        _screenPlacedSprite;
        ProgramSptr        _worldPlacedLabel;
        ProgramSptr        _screenPlacedLabel;

        friend class rasterizer::Billboard;
    };
}
