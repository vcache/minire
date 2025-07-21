#pragma once

#include <rasterizer/coordinates.hpp>
#include <rasterizer/drawable.hpp>
#include <rasterizer/fonts.hpp>
#include <rasterizer/label.hpp>
#include <rasterizer/labels.hpp>
#include <rasterizer/lines.hpp>
#include <rasterizer/materials.hpp>
#include <rasterizer/meshes.hpp>
#include <rasterizer/sprites.hpp>
#include <rasterizer/textures.hpp>
#include <rasterizer/ubo.hpp>
#include <scene.hpp>
#include <utils/viewpoint.hpp>

#include <glm/mat4x4.hpp>

namespace minire::content { class Manager; }

namespace minire
{
    class Rasterizer
    {
    public:
        explicit Rasterizer(content::Manager &,
                            content::Ids const & fontsPreload = {});

        void draw(utils::Viewpoint const &,
                  Scene const & scene);

        void setScreenSize(float w, float h);

    public:
        rasterizer::Labels & labels() { return _labels; }
        rasterizer::Sprites & sprites() { return _sprites; }
        rasterizer::Meshes & meshes() { return _meshes; }
        rasterizer::Lines & lines() { return _lines; }

    private:
        void draw3d(utils::Viewpoint const & viewpoint,
                    Scene const & scene);
        void draw2d();

    private:
        content::Manager             & _contentManager;

        // NOTE: the order of these is ridiculously vital (see ctor)
        rasterizer::Ubo                _ubo;
        rasterizer::Coordinates        _coordinates;
        rasterizer::Lines              _lines;
        rasterizer::Textures           _textures;
        rasterizer::Materials          _materials;
        rasterizer::Meshes             _meshes;
        rasterizer::Fonts              _fonts;
        rasterizer::Labels             _labels;
        rasterizer::Sprites            _sprites;

        glm::mat4                      _2dProjection;
        rasterizer::Drawable::PtrsList _drawables;
        size_t                         _modelsUsage;
    };
}
