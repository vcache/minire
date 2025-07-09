#include <rasterizer.hpp>

#include <minire/content/manager.hpp>
#include <opengl.hpp>

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
        , _textures(_contentManager)
        , _models(_ubo, _textures, _contentManager)
        , _fonts(_contentManager, fontsPreload)
        , _labels(_fonts)
        , _sprites(_textures)
        , _2dProjection(1.0)
    {
        // TODO: preload textures for sprites
   }

    void Rasterizer::setScreenSize(float w, float h)
    {
        _2dProjection = glm::ortho(0.0f, w, 0.0f, h);
    }

    void Rasterizer::draw(utils::Viewpoint const & viewpoint,
                      Scene const & scene)
    {
        // update and bind UBO
        {
            glm::mat4 const & transform = viewpoint.transform();
            size_t transformVersion = viewpoint.transformVersion();
            _ubo.setViewProjection(transform, transformVersion);
            _ubo.setViewPosition(glm::vec4(viewpoint.position(), 1.0f));
            _ubo.setLights(scene.cullPointLights(viewpoint, rasterizer::Ubo::maxLights()));
            _ubo.bind();
        }

        draw3d(viewpoint, scene);
        draw2d();
    }

    void Rasterizer::draw3d(utils::Viewpoint const & viewpoint,
                            Scene const & scene)
    {
        // setup state for 3d mode
        MINIRE_GL(glEnable, GL_CULL_FACE)
        MINIRE_GL(glEnable, GL_DEPTH_TEST); 
        MINIRE_GL(glDepthFunc, GL_LESS);
        //MINIRE_GL(glCullFace, GL_FRONT);
        MINIRE_GL(glDisable, GL_BLEND);
        MINIRE_GL(glBlendFunc, GL_ONE, GL_ZERO);

        // draw coordinates
        _coordinates.draw();

        // draw debug lines
        _lines.draw();

        // draw entries
        scene::ModelRef::List models = scene.cullModels(viewpoint);
        _models.draw(models);
    }

    void Rasterizer::draw2d()
    {
        // disable depth test and blending
        MINIRE_GL(glDisable, GL_DEPTH_TEST);
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

        // TODO: avoid sorting, use Z-buffer instead
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
