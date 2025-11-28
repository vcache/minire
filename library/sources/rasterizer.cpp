#include <rasterizer.hpp>

#include <minire/content/manager.hpp>
#include <minire/models/pbr-material.hpp>

#include <opengl.hpp>
#include <rasterizer/materials/pbr.hpp>
#include <scene.hpp>

#include <glm/gtx/transform.hpp>

#include <algorithm>
#include <cassert>

namespace minire
{
    Rasterizer::Rasterizer(content::Manager & contentManager,
                           content::Ids const & fontsPreload)
        : _contentManager(contentManager)
        , _ubo()
        , _coordinates(_ubo)
        , _lines(_ubo)
        , _textures(_contentManager, _resources)
        , _materials()
        , _vertexBuffers(_resources)
        , _meshes(_ubo, _materials, _vertexBuffers, _contentManager, _resources)
        , _fonts(_contentManager, fontsPreload)
        , _labels(_fonts)
        , _sprites(_textures)
        , _billboards(_contentManager, _fonts, _textures)
        , _2dProjection(1.0)
        , _screenWidth(0)
        , _screenHeight(0)
    {
        // TODO: preload textures for sprites

        _materials.add(models::PbrMaterial::kMaterialKind,
                       std::make_unique<rasterizer::materials::PbrFactory>(_textures));
   }

    void Rasterizer::setScreenSize(size_t const width,
                                   size_t const height)
    {
        //_2dProjection = glm::ortho(0.0f, w, 0.0f, h);
        _2dProjection = glm::ortho(0.0f,
                                   static_cast<float>(width),
                                   static_cast<float>(height),
                                   0.0f);
        _screenWidth = width;
        _screenHeight = height;
    }

    void Rasterizer::draw(Scene const & scene)
    {
        forwardPass(scene);
    }

    void Rasterizer::forwardPass(Scene const & scene)
    {
        // initial setup
        assert(_screenWidth != 0);
        assert(_screenHeight != 0);
        MINIRE_GL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
        MINIRE_GL(glViewport, 0, 0, _screenWidth, _screenHeight);
        MINIRE_GL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 3D part
        if (scene::Viewpoint const & viewpoint = scene.viewpoint();
            viewpoint.hasCamera())
        {
            assert(viewpoint.width() == _screenWidth);
            assert(viewpoint.height() == _screenHeight);

            auto const & [mvp, revision] = viewpoint.mvp();
            _ubo.setViewProjection(mvp, revision);
            _ubo.setViewPosition(glm::vec4(viewpoint.position(), 1.0f));
            _ubo.setLights(scene);
            _ubo.bind();
            draw3d(scene);
        }

        // 2D layer
        draw2d();
    }

    void Rasterizer::draw3d(Scene const & scene)
    {
        // setup state for 3d mode
        MINIRE_GL(glEnable, GL_CULL_FACE);
        //MINIRE_GL(glCullFace, GL_FRONT);
        MINIRE_GL(glEnable, GL_MULTISAMPLE);
        MINIRE_GL(glEnable, GL_DEPTH_TEST);
        MINIRE_GL(glDepthFunc, GL_LESS);
        MINIRE_GL(glDepthMask, GL_TRUE);
        MINIRE_GL(glDisable, GL_BLEND);
        MINIRE_GL(glBlendFunc, GL_ONE, GL_ZERO);

        // draw coordinates
        _coordinates.draw();

        // draw debug lines
        _lines.draw();

        // draw entries
        _meshes.draw(scene);

        // draw billboards
        MINIRE_GL(glDisable, GL_CULL_FACE);
        MINIRE_GL(glDepthFunc, GL_LEQUAL);
        // TODO: fix alpha blending for billboards
        //MINIRE_GL(glEnable, GL_BLEND);
        //MINIRE_GL(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        _billboards.draw(scene);
    }

    void Rasterizer::draw2d()
    {
        // disable depth test and blending
        MINIRE_GL(glDisable, GL_DEPTH_TEST);
        MINIRE_GL(glDisable, GL_CULL_FACE);
        MINIRE_GL(glDisable, GL_MULTISAMPLE);
        MINIRE_GL(glEnable, GL_BLEND);
        MINIRE_GL(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // TODO: dont Rasterizer parts that will be filled 2d items
        //       (maybe Rasterizer 2D first and update Z-buffer to max
        //        or use stencil for buffer for 2Ds)
        //      - (that will kill blending for 2Ds)

        // TODO: group by texture

        _drawables.clear();
        _labels.predraw(_drawables);
        _sprites.predraw(_drawables);

        // TODO: avoid sorting, use Z-buffer instead OR zOrder invalidation
        std::sort(_drawables.begin(), _drawables.end(),
            [](rasterizer::Drawable const * a, rasterizer::Drawable const * b)
            {
                assert(a);
                assert(b);
                return a->zOrder() < b->zOrder();
            });

        for(rasterizer::Drawable const * drawable : _drawables)
        {
            assert(drawable);
            drawable->draw(_2dProjection);
        }
    }
}
