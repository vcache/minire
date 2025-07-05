#pragma once

#include <gpu/render/coordinates.hpp>
#include <gpu/render/drawable.hpp>
#include <gpu/render/fonts.hpp>
#include <gpu/render/label.hpp>
#include <gpu/render/labels.hpp>
#include <gpu/render/lines.hpp>
#include <gpu/render/models.hpp>
#include <gpu/render/sprites.hpp>
#include <gpu/render/textures.hpp>
#include <gpu/render/ubo.hpp>
#include <scene.hpp>
#include <utils/viewpoint.hpp>

#include <glm/mat4x4.hpp>

namespace minire::content { class Manager; }

namespace minire::gpu
{
    class Render
    {
    public:
        explicit Render(content::Manager &,
                        content::Ids const & fonstPreload = {});

        void draw(utils::Viewpoint const &,
                  Scene const & scene);

        void setScreenSize(float w, float h);

    public:
        render::Labels & labels() { return _labels; }
        render::Sprites & sprites() { return _sprites; }
        render::Models & models() { return _models; }
        render::Lines & lines() { return _lines; }

    private:
        void draw3d(utils::Viewpoint const & viewpoint,
                    Scene const & scene);
        void draw2d();

    private:
        content::Manager       &   _contentManager;

        render::Ubo                _ubo;
        render::Coordinates        _coordinates;
        render::Lines              _lines;
        render::Textures           _textures;
        render::Models             _models;
        render::Fonts              _fonts;
        render::Labels             _labels;
        render::Sprites            _sprites;

        glm::mat4                  _2dProjection;
        render::Drawable::PtrsList _drawables;
        size_t                     _modelsUsage;
    };
}
