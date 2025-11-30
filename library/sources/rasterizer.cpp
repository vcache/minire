#include <rasterizer.hpp>

#include <minire/content/manager.hpp>
#include <minire/models/pbr-material.hpp>

#include <opengl.hpp>
#include <rasterizer/materials/pbr.hpp>
#include <scene.hpp>
#include <utils/frustum.hpp>

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
        , _directionalLightsShadowMaps(rasterizer::Ubo::maxDirectionalLights(), nullptr)
        , _pointLightsShadowMaps(rasterizer::Ubo::maxPointLights(), nullptr)
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

    rasterizer::CulledDirectionalLights
    Rasterizer::cullDirectionalLights(Scene const & scene)
    {
        rasterizer::CulledDirectionalLights result;
        result.reserve(rasterizer::Ubo::maxDirectionalLights());
        size_t shadowMapIndex = 0;
        scene.cullDirectionalLights(
            rasterizer::Ubo::maxDirectionalLights(),
            [this, &result, &shadowMapIndex](size_t const /*index*/,
                                             glm::vec3 const & position,
                                             glm::vec3 const & direction,
                                             glm::vec3 const & color,
                                             models::MaybeShadowParams const & shadowParams)
            {
                rasterizer::FlatShadowMap::Sptr shadowMap;
                bool usePCF = false;

                if (shadowParams)
                {
                    if (shadowMapIndex >= _flatShadowMaps.size())
                    {
                        _flatShadowMaps.push_back(
                            std::make_shared<rasterizer::FlatShadowMap>(shadowParams->_mapSize));
                    }

                    assert(_flatShadowMaps[shadowMapIndex]);
                    if (_flatShadowMaps[shadowMapIndex]->size() != shadowParams->_mapSize)
                    {
                        // TODO: should try to lookup for an existing map with a given size
                        MINIRE_WARNING("flat shadow map have to be rebuild, due to size change from {} to {}",
                                       _flatShadowMaps[shadowMapIndex]->size(), shadowParams->_mapSize);
                        _flatShadowMaps[shadowMapIndex] =
                            std::make_shared<rasterizer::FlatShadowMap>(shadowParams->_mapSize);
                    }

                    shadowMap = _flatShadowMaps[shadowMapIndex];
                    usePCF = shadowParams->_usePCF;

                    shadowMapIndex++;
                }

                result.emplace_back(rasterizer::CulledDirectionalLight
                {
                    ._position = position,
                    ._direction = direction,
                    ._color = color,
                    ._shadowMap = shadowMap,
                    ._viewProjection = glm::identity<glm::mat4>(),
                    ._shadowUsePCF = usePCF,
                });
            });
        return result;
    }

    rasterizer::CulledPointLights
    Rasterizer::cullPointLights(Scene const & scene)
    {
        rasterizer::CulledPointLights result;
        result.reserve(rasterizer::Ubo::maxPointLights());
        size_t shadowMapIndex = 0;
        scene.cullPointLights(
            rasterizer::Ubo::maxPointLights(),
            [this, &result, &shadowMapIndex](size_t const /*index*/,
                                             glm::vec3 const & position,
                                             glm::vec4 const & color,
                                             glm::vec4 const & attenuation,
                                             models::MaybeShadowParams const & shadowParams)
            {
                rasterizer::CubeShadowMap::Sptr shadowMap;
                bool usePCF = false;

                if (shadowParams)
                {
                    if (shadowMapIndex >= _cubeShadowMaps.size())
                    {
                        _cubeShadowMaps.push_back(
                            std::make_shared<rasterizer::CubeShadowMap>(shadowParams->_mapSize));
                    }

                    assert(_cubeShadowMaps[shadowMapIndex]);
                    if (_cubeShadowMaps[shadowMapIndex]->size() != shadowParams->_mapSize)
                    {
                        // TODO: should try to lookup for an existing map with a given size
                        MINIRE_WARNING("cube shadow map have to be rebuild, due to size change from {} to {}",
                                       _cubeShadowMaps[shadowMapIndex]->size(), shadowParams->_mapSize);
                        _cubeShadowMaps[shadowMapIndex] =
                            std::make_shared<rasterizer::CubeShadowMap>(shadowParams->_mapSize);
                    }

                    shadowMap = _cubeShadowMaps[shadowMapIndex];
                    usePCF = shadowParams->_usePCF;

                    shadowMapIndex++;
                }

                result.emplace_back(rasterizer::CulledPointLight
                {
                    ._position = position,
                    ._color = color,
                    ._attenuation = attenuation,
                    ._shadowMap = shadowMap,
                    ._shadowMapFarPlane = 0,
                    ._shadowUsePCF = usePCF,
                });
            });
        return result;
    }

    void Rasterizer::draw(Scene const & scene)
    {
        auto directionalLights = cullDirectionalLights(scene);
        auto pointLights = cullPointLights(scene);
        shadowPass(scene, directionalLights, pointLights);
        colorPass(scene, directionalLights, pointLights);
    }

    void Rasterizer::shadowPass(Scene const & scene,
                                rasterizer::CulledDirectionalLights & culledDirectionalLights,
                                rasterizer::CulledPointLights & culledPointLights)
    {
        // TODO: maybe do min/max w/ scene AABB?
        utils::FrustumVertices const & frustumVertices = scene.viewpoint().frustumVertices();

        // build shadow maps for directional lights (if any)
        for(rasterizer::CulledDirectionalLight & light : culledDirectionalLights)
        {
            if (light._shadowMap)
            {
                light._viewProjection = light._shadowMap->perform(
                    scene, light._position, light._direction, frustumVertices);
            }
            else
            {
                light._viewProjection = glm::identity<glm::mat4>();
            }
        }

        // build shadow maps for point lights (if any)
        for(rasterizer::CulledPointLight & light : culledPointLights)
        {
            if (light._shadowMap)
            {
                light._shadowMapFarPlane = light._shadowMap->perform(
                    scene, light._position, frustumVertices);
            }
            else
            {
                light._shadowMapFarPlane = 0;
            }
        }
    }

    void Rasterizer::colorPass(Scene const & scene,
                               rasterizer::CulledDirectionalLights const & culledDirectionalLights,
                               rasterizer::CulledPointLights const & culledPointLights)
    {
        // initial setup
        assert(_screenWidth != 0);
        assert(_screenHeight != 0);
        MINIRE_GL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
        MINIRE_GL(glViewport, 0, 0, _screenWidth, _screenHeight);
        MINIRE_GL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // actualize shadow maps vector for directional lights
        _directionalLightsShadowMaps.resize(culledDirectionalLights.size());
        for(size_t i = 0; i < _directionalLightsShadowMaps.size(); i++)
        {
            if (culledDirectionalLights[i]._shadowMap)
            {
                auto const & flatShadowMap = *culledDirectionalLights[i]._shadowMap;
                _directionalLightsShadowMaps[i] = &flatShadowMap.texture();
            }
            else
            {
                _directionalLightsShadowMaps[i] = nullptr;
            }
        }

        // actualize shadow maps vector for point lights
        _pointLightsShadowMaps.resize(culledPointLights.size());
        for(size_t i = 0; i < _pointLightsShadowMaps.size(); i++)
        {
            if (culledPointLights[i]._shadowMap)
            {
                auto const & cubeShadowMap = *culledPointLights[i]._shadowMap;
                _pointLightsShadowMaps[i] = &cubeShadowMap.texture();
            }
            else
            {
                _pointLightsShadowMaps[i] = nullptr;
            }
        }

        // perform drawing of 3D layer
        if (scene::Viewpoint const & viewpoint = scene.viewpoint();
            viewpoint.hasCamera())
        {
            assert(viewpoint.width() == _screenWidth);
            assert(viewpoint.height() == _screenHeight);

            auto const & [mvp, revision] = viewpoint.mvp();
            _ubo.setViewProjection(mvp, revision);
            _ubo.setViewPosition(glm::vec4(viewpoint.position(), 1.0f));
            _ubo.setLights(culledDirectionalLights, culledPointLights);
            _ubo.bind();
            draw3d(scene, _directionalLightsShadowMaps,
                   _pointLightsShadowMaps);
        }

        // perform drawing of 2D layer
        draw2d();
    }

    void Rasterizer::draw3d(Scene const & scene,
                            material::TextureRefs const & directionalLightsShadowMaps,
                            material::TextureRefs const & pointLightsShadowMaps)
    {
        // setup state for 3d mode
        MINIRE_GL(glEnable, GL_CULL_FACE);
        MINIRE_GL(glCullFace, GL_BACK);
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
        _meshes.draw(scene, directionalLightsShadowMaps,
                     pointLightsShadowMaps);

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
