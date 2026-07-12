#pragma once

#include <opengl/fbo.hpp>
#include <opengl/pbo.hpp>
#include <opengl/program.hpp>
#include <opengl/rbo.hpp>
#include <opengl/texture.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>
#include <rasterizer/billboards.hpp>
#include <rasterizer/coordinates.hpp>
#include <rasterizer/cube-shadow-map.hpp>
#include <rasterizer/culled-objects.hpp>
#include <rasterizer/drawable.hpp>
#include <rasterizer/flat-shadow-map.hpp>
#include <rasterizer/fonts.hpp>
#include <rasterizer/instanced-buffers.hpp>
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

#ifndef NDEBUG
#   include <minire/instrumentation/histogram.hpp>
#endif

#include <glm/mat4x4.hpp>

namespace minire::content { class Manager; }

namespace minire
{
    class SceneImpl;

    class Rasterizer
    {
    public:
        explicit Rasterizer(content::Manager &,
                            int width, int height,
                            content::Ids const & fontsPreload = {});

        ~Rasterizer();

        void draw(SceneImpl const &);

        void setScreenSize(size_t const width,
                           size_t const height);

        // NOTE: might have some latency (reads from pixel buffer)
        uint32_t fetchMeshId(size_t const x, size_t const y) const;

        // NOTE: Shouldn't have any latencies
        uint32_t fetchHotMeshId() const;
        void setHotFragment(size_t const x, size_t const y);

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
        rasterizer::CulledDirectionalLights cullDirectionalLights(SceneImpl const &);
        rasterizer::CulledPointLights cullPointLights(SceneImpl const &);
        rasterizer::CulledPrimitives cullPrimitives(SceneImpl const &);

    private:
        void shadowPass(SceneImpl const &,
                        rasterizer::CulledPrimitives const &,
                        rasterizer::CulledDirectionalLights &,
                        rasterizer::CulledPointLights &);

        void colorPass(SceneImpl const &,
                       rasterizer::CulledPrimitives const &,
                       rasterizer::CulledDirectionalLights const &,
                       rasterizer::CulledPointLights const &);
        void draw3d(SceneImpl const &,
                    rasterizer::CulledPrimitives const &,
                    material::TextureRefs const &,
                    material::TextureRefs const &);
        void draw2d();

    private:
        using FlatShadowMaps = std::vector<rasterizer::FlatShadowMap::Sptr>;
        using CubeShadowMaps = std::vector<rasterizer::CubeShadowMap::Sptr>;

        class ScreenPassUbo;

    private:
        content::Manager               & _contentManager;

        // Object required fo Multi-Render Target (MRT)
        opengl::FBO                      _primaryFbo;
        opengl::Texture::Uptr            _colorBuffer;
        opengl::Texture::Uptr            _idBuffer;
        opengl::RBO::Uptr                _depthRbo;
        opengl::VAO                      _screenQuadVao;
        opengl::VBO                      _screenQuadVbo;
        opengl::Program                  _screenQuadProgram;
        GLint const                      _screenTextureUniform;
        GLint const                      _idTextureUniform;
        GLint const                      _outlineIdsUniform;
        GLint const                      _outlineIdsCountUniform;
        opengl::PBO::Uptr                _hotFragmentPbo; // PBO for ID under the cursor (a hot fragment)
        size_t                           _hotFragmentX = 0;
        size_t                           _hotFragmentY = 0;
        std::unique_ptr<ScreenPassUbo>   _screenPassUbo;

        // NOTE: the order of these is ridiculously vital (see ctor)
        rasterizer::Ubo                  _ubo;

        rasterizer::InstancedBuffersPool _instancedBuffersPool;
        rasterizer::Coordinates          _coordinates;
        rasterizer::Lines                _lines;
        rasterizer::Textures             _textures;
        rasterizer::Materials            _materials;
        rasterizer::VertexBuffers        _vertexBuffers;
        rasterizer::Meshes               _meshes;
        rasterizer::Fonts                _fonts;
        rasterizer::Labels               _labels;
        rasterizer::Sprites              _sprites;
        rasterizer::Billboards           _billboards;

        FlatShadowMaps                   _flatShadowMaps;
        material::TextureRefs            _directionalLightsShadowMaps;

        CubeShadowMaps                   _cubeShadowMaps;
        material::TextureRefs            _pointLightsShadowMaps;

        rasterizer::Resources            _resources;

        glm::mat4                        _2dProjection;
        rasterizer::Drawable::PtrsList   _drawables;
        size_t                           _modelsUsage;

        size_t                           _screenWidth;
        size_t                           _screenHeight;

#       ifndef NDEBUG
        instrumentation::Histogram<>    _statistics;
#       endif

    };
}
