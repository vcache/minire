#pragma once

#include <rasterizer/billboards.hpp>
#include <rasterizer/coordinates.hpp>
#include <rasterizer/culled-objects.hpp>
#include <rasterizer/drawable.hpp>
#include <rasterizer/flat-shadow-map.hpp>
#include <rasterizer/fonts.hpp>
#include <rasterizer/label.hpp>
#include <rasterizer/labels.hpp>
#include <rasterizer/lines.hpp>
#include <rasterizer/materials.hpp>
#include <rasterizer/meshes.hpp>
#include <rasterizer/resources.hpp>
#include <rasterizer/sprites.hpp>
#include <rasterizer/textures.hpp>
#include <rasterizer/ubo.hpp>
#include <rasterizer/vertex-buffers.hpp>

#include <minire/material.hpp>

#include <glm/mat4x4.hpp>

namespace minire::content { class Manager; }

namespace minire
{
    class Scene;

    class Rasterizer
    {
    public:
        explicit Rasterizer(content::Manager &,
                            content::Ids const & fontsPreload = {});

        void draw(Scene const &);

        void setScreenSize(size_t const width,
                           size_t const height);

    public:
        rasterizer::Labels & labels() { return _labels; }
        rasterizer::Sprites & sprites() { return _sprites; }
        rasterizer::Meshes & meshes() { return _meshes; }
        rasterizer::Lines & lines() { return _lines; }
        rasterizer::VertexBuffers & vertexBuffers() { return _vertexBuffers; }
        rasterizer::Billboards & billboards() { return _billboards; }

    public:
        void newResourceLayer(rasterizer::Resources::LayerId const & layerId) { _resources.newLayer(layerId); }
        void disposeResourceLayer(rasterizer::Resources::LayerId const & layerId) { _resources.disposeLayer(layerId); }
        rasterizer::Resources::LayerId const & currentResourceLayer() const { return _resources.current(); }

    private:
        rasterizer::CulledDirectionalLights cullDirectionalLights(Scene const &);

    private:
        void shadowPass(Scene const &, rasterizer::CulledDirectionalLights &);

        void colorPass(Scene const &, rasterizer::CulledDirectionalLights &);
        void draw3d(Scene const &, material::TextureRefs const &);
        void draw2d();

    private:
        using FlatShadowMaps = std::vector<rasterizer::FlatShadowMap::Sptr>;

    private:
        content::Manager             & _contentManager;

        // NOTE: the order of these is ridiculously vital (see ctor)
        rasterizer::Ubo                _ubo;

        rasterizer::Coordinates        _coordinates;
        rasterizer::Lines              _lines;
        rasterizer::Textures           _textures;
        rasterizer::Materials          _materials;
        rasterizer::VertexBuffers      _vertexBuffers;
        rasterizer::Meshes             _meshes;
        rasterizer::Fonts              _fonts;
        rasterizer::Labels             _labels;
        rasterizer::Sprites            _sprites;
        rasterizer::Billboards         _billboards;

        FlatShadowMaps                 _flatShadowMaps;
        material::TextureRefs          _directionalLightsShadowMaps;

        rasterizer::Resources          _resources;

        glm::mat4                      _2dProjection;
        rasterizer::Drawable::PtrsList _drawables;
        size_t                         _modelsUsage;

        size_t                         _screenWidth;
        size_t                         _screenHeight;
    };
}
