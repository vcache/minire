#include <rasterizer.hpp>

#include <minire/content/manager.hpp>
#include <minire/logging.hpp>
#include <minire/models/pbr-material.hpp>
#include <minire/models/shadow-params.hpp>

#include <opengl.hpp>
#include <opengl/shader.hpp>
#include <opengl/ubo.hpp>
#include <rasterizer/binding-points.hpp>
#include <rasterizer/materials/pbr.hpp>
#include <scene-impl.hpp>
#include <utils/frustum.hpp>

#include <glm/gtx/transform.hpp>

#include <algorithm>
#include <cassert>

namespace minire
{
    namespace
    {
        static float const kScreenQuad[] =
        {
            // xy           // uv
            -1.0f,  1.0f,   0.0f, 1.0f,
            -1.0f, -1.0f,   0.0f, 0.0f,
             1.0f, -1.0f,   1.0f, 0.0f,

            -1.0f,  1.0f,   0.0f, 1.0f,
             1.0f, -1.0f,   1.0f, 0.0f,
             1.0f,  1.0f,   1.0f, 1.0f
        };

        static constexpr GLsizei kScreenQuadStride = sizeof(float) * (2 + 2);
        static constexpr size_t kMaxPixelOutlines = 32; // NOTE: see kFragShader
        static constexpr size_t kStd140N = 4;

        struct alignas(1 * kStd140N) PixelOutline
        {
            alignas(4 * kStd140N) glm::vec4 _color = glm::vec4(0);
            alignas(1 * kStd140N) uint32_t  _id = 0;
        };

        struct alignas(1 * kStd140N) ScreenPassDatablock
        {
            alignas(4 * kStd140N) PixelOutline _pixelOutlines[kMaxPixelOutlines];
            alignas(1 * kStd140N) uint32_t     _pixelOutlineCount = 0;
        };

        static char const * kVertShader =
        R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoords;
            out vec2 TexCoords;
            void main()
            {
                TexCoords = aTexCoords;
                gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
            }
        )";

        static char const * kFragShader =
        R"(
            #version 330 core

            struct PixelOutline
            {
                vec4 _color;
                uint _id;
            };

            layout(std140) uniform kScreenPassDatablock
            {
                PixelOutline _pixelOutlines[32];  // NOTE: see kMaxPixelOutlines
                uint         _pixelOutlineCount;
            };

            in vec2 TexCoords;
            out vec4 FragColor;

            uniform sampler2D screenTexture;
            uniform usampler2D idTexture;

            bool isSelected(uint id, out uint outlineIndex)
            {
                for(uint i = uint(0); i < _pixelOutlineCount; i++)
                {
                    if (_pixelOutlines[i]._id == id)
                    {
                        outlineIndex = i;
                        return true;
                    }
                }
                return false;
            }

            bool edgeTest(out vec4 color)
            {
                uint currentId = texture(idTexture, TexCoords).r;
                uint outlineIndex;
                if (!isSelected(currentId, outlineIndex))
                {
                    for(int x = -1; x <= 1; x++)
                    {
                        for(int y = -1; y <= 1; y++)
                        {
                            uint offsetObjId = textureOffset(idTexture, TexCoords, ivec2(x, y)).r;
                            if(isSelected(offsetObjId, outlineIndex))
                            {
                                color = _pixelOutlines[outlineIndex]._color;
                                return true;
                            }
                        }
                    }
                }
                return false;
            }

            void main()
            {
                // base color
                FragColor = texture(screenTexture, TexCoords);

                // optional outline
                if (_pixelOutlineCount != uint(0))
                {
                    vec4 outlineColor;
                    if (edgeTest(outlineColor))
                    {
                        FragColor.rgb = mix(FragColor.rgb, outlineColor.rgb, outlineColor.a);
                    }
                }
            }
        )";
    }

    // Rasterizer::ScreenPassUbo //

    class Rasterizer::ScreenPassUbo
    {
        ScreenPassUbo(ScreenPassUbo const &) = delete;
        ScreenPassUbo & operator=(ScreenPassUbo const &) = delete;
        ScreenPassUbo(ScreenPassUbo &&) = delete;
        ScreenPassUbo & operator=(ScreenPassUbo &&) = delete;

    public:
        explicit ScreenPassUbo(opengl::Program & program)
        {
            // Bind a uniform to a binding point
            GLuint const datablockIndex = program.getUniformBlockIndex("kScreenPassDatablock");
            assert(GL_INVALID_INDEX != datablockIndex);
            MINIRE_GL(glUniformBlockBinding, program.id(), datablockIndex,
                      rasterizer::kScreenPassBindingPoint);

            // Bind a UBO buffer to the same binding point
            _ubo.bindBufferBase(rasterizer::kScreenPassBindingPoint);
        }

        ScreenPassDatablock & datablock() { return _datablock; }

        void update()
        {
            _ubo.update(_datablock);
        }

    private:
        ScreenPassDatablock              _datablock;
        opengl::UBO<ScreenPassDatablock> _ubo;
    };

    // Rasterizer //

    Rasterizer::Rasterizer(content::Manager & contentManager,
                           int width, int height,
                           content::Ids const & fontsPreload)
        : _contentManager(contentManager)
        , _screenQuadProgram({
            std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, kVertShader),
            std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader),
        })
        , _screenTextureUniform(_screenQuadProgram.getUniformLocation("screenTexture"))
        , _idTextureUniform(_screenQuadProgram.getUniformLocation("idTexture"))
        , _outlineIdsUniform(_screenQuadProgram.getUniformLocation("outlineIds"))
        , _outlineIdsCountUniform(_screenQuadProgram.getUniformLocation("outlineIdsCount"))
        , _screenPassUbo(std::make_unique<ScreenPassUbo>(_screenQuadProgram))
        , _ubo()
        , _coordinates(_ubo)
        , _lines(_ubo)
        , _textures(_contentManager, _resources)
        , _materials()
        , _vertexBuffers(_resources)
        , _meshes(_ubo, _materials, _vertexBuffers, _contentManager, _resources)
        , _fonts(_contentManager, fontsPreload)
        , _labels(_fonts, _contentManager)
        , _sprites(_textures)
        , _billboards(_contentManager, _fonts, _textures)
        , _directionalLightsShadowMaps(rasterizer::Ubo::maxDirectionalLights(), nullptr)
        , _pointLightsShadowMaps(rasterizer::Ubo::maxPointLights(), nullptr)
        , _2dProjection(1.0)
        , _screenWidth(static_cast<size_t>(width))
        , _screenHeight(static_cast<size_t>(height))
    {
        // TODO: preload textures for sprites

        MINIRE_INVARIANT(width >= 0 && height >= 0, "bad window size: {}x{}", width, height);

        _materials.add(models::PbrMaterial::kMaterialKind,
                       std::make_unique<rasterizer::materials::PbrFactory>(_textures));

        // Build main FBO

        _primaryFbo.bind();

        {
            _colorBuffer = std::make_unique<opengl::Texture>(GL_TEXTURE_2D);
            MINIRE_GL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA8, width, height,
                      0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // drop mipmap filter
            _colorBuffer->parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            _colorBuffer->parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            _colorBuffer->parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            _colorBuffer->parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            _primaryFbo.attach2D(*_colorBuffer, GL_COLOR_ATTACHMENT0);
        }

        {
            _idBuffer = std::make_unique<opengl::Texture>(GL_TEXTURE_2D);
            MINIRE_GL(glTexImage2D, GL_TEXTURE_2D, 0, GL_R32UI, width, height,
                      0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
            _primaryFbo.attach2D(*_idBuffer, GL_COLOR_ATTACHMENT1);

            _idBuffer->parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            _idBuffer->parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            _idBuffer->parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            _idBuffer->parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        GLenum const drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        MINIRE_GL(glDrawBuffers, 2, drawBuffers);

        {
            _depthRbo = std::make_unique<opengl::RBO>();
            _depthRbo->storage(width, height, GL_DEPTH_COMPONENT24);
            _primaryFbo.attach(*_depthRbo, GL_DEPTH_ATTACHMENT);
        }

        MINIRE_INVARIANT(::glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                         "primary FBO isn't complete");

        _primaryFbo.unbind();

        // Build PBO

        {
            _hotFragmentPbo = std::make_unique<opengl::PBO>();
            _hotFragmentPbo->bufferData(sizeof(uint32_t), NULL, GL_STREAM_READ);
            _hotFragmentPbo->unbind();
        }

        // Build screen quad

        _screenQuadVao = std::make_shared<opengl::VAO>();

        _screenQuadVbo = std::make_shared<opengl::VBO>(_screenQuadVao, GL_ARRAY_BUFFER);
        _screenQuadVbo->bufferData(sizeof(kScreenQuad), kScreenQuad, GL_STATIC_DRAW);

        _screenQuadVao->enableAttrib(0);
        _screenQuadVao->attribPointer(0, 2, GL_FLOAT, GL_FALSE, kScreenQuadStride, 0);

        _screenQuadVao->enableAttrib(1);
        _screenQuadVao->attribPointer(1, 2, GL_FLOAT, GL_FALSE, kScreenQuadStride, 2 * sizeof(float));

        // Initial call

        setScreenSize(_screenWidth, _screenHeight);
    }

    Rasterizer::~Rasterizer() = default;

    void Rasterizer::setScreenSize(size_t const width,
                                   size_t const height)
    {
        //_2dProjection = glm::ortho(0.0f, w, 0.0f, h);
        _2dProjection = glm::ortho(0.0f,
                                   static_cast<float>(width),
                                   static_cast<float>(height),
                                   0.0f);

        bool const changed = width != _screenWidth || height != _screenHeight;
        if (changed)
        {
            // Color buffer
            assert(_colorBuffer);
            _colorBuffer->bind();
            MINIRE_GL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA8, width, height,
                      0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // ID buffer
            assert(_idBuffer);
            _idBuffer->bind();
            MINIRE_GL(glTexImage2D, GL_TEXTURE_2D, 0, GL_R32UI, width, height,
                      0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

            // Depth buffer
            assert(_depthRbo);
            _depthRbo->storage(width, height, GL_DEPTH_COMPONENT24);
        }

        _screenWidth = width;
        _screenHeight = height;
    }

    void Rasterizer::setHotFragment(size_t const x, size_t const y)
    {
        _hotFragmentX = x;
        _hotFragmentY = y;
    }

    uint32_t Rasterizer::fetchMeshId(size_t const x, size_t const y) const
    {
        if (x >= _screenWidth || y >= _screenHeight)
            return 0;

        // make sure it not conflicts w/ _hotFragmentPbo
        assert(_hotFragmentPbo);
        _hotFragmentPbo->unbind();

        _primaryFbo.bind(GL_READ_FRAMEBUFFER);
        MINIRE_GL(glReadBuffer, GL_COLOR_ATTACHMENT1);

        uint32_t pixelData = 0;
        glReadPixels(x, _screenHeight - 1 - y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &pixelData);

        _primaryFbo.unbind();    // will bind to 0

        return pixelData;
    }

    uint32_t Rasterizer::fetchHotMeshId() const
    {
        uint32_t id = 0;

        assert(_hotFragmentPbo);
        if (opengl::PBO::Mapping mapping = _hotFragmentPbo->mapBuffer();
            mapping)
        {
            id = *mapping.dataAs<uint32_t>();
        }

        _hotFragmentPbo->unbind();
        return id;
    }

    rasterizer::CulledDirectionalLights
    Rasterizer::cullDirectionalLights(SceneImpl const & scene)
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

                if (shadowParams)
                {
                    if (shadowMapIndex >= _flatShadowMaps.size())
                    {
                        _flatShadowMaps.push_back(
                            std::make_shared<rasterizer::FlatShadowMap>(*shadowParams));
                    }

                    assert(_flatShadowMaps[shadowMapIndex]);
                    if (_flatShadowMaps[shadowMapIndex]->shadowParams() != *shadowParams)
                    {
                        // TODO: should try to lookup for an existing map with a compatible parameters
                        // TODO: maybe "hard-attach" maps to lights so that very customized shaders
                        //       can be generated (i.e. shaders without branching)
                        MINIRE_WARNING("flat shadow map have to be rebuild");
                        _flatShadowMaps[shadowMapIndex] =
                            std::make_shared<rasterizer::FlatShadowMap>(*shadowParams);
                    }

                    shadowMap = _flatShadowMaps[shadowMapIndex];

                    shadowMapIndex++;
                }

                result.emplace_back(rasterizer::CulledDirectionalLight
                {
                    ._position = position,
                    ._direction = direction,
                    ._color = color,
                    ._shadowMap = shadowMap,
                    ._viewProjection = glm::identity<glm::mat4>(),
                });
            });
        return result;
    }

    rasterizer::CulledPointLights
    Rasterizer::cullPointLights(SceneImpl const & scene)
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

                if (shadowParams)
                {
                    if (shadowMapIndex >= _cubeShadowMaps.size())
                    {
                        _cubeShadowMaps.push_back(
                            std::make_shared<rasterizer::CubeShadowMap>(*shadowParams));
                    }

                    assert(_cubeShadowMaps[shadowMapIndex]);
                    if (_cubeShadowMaps[shadowMapIndex]->shadowParams() != *shadowParams)
                    {
                        // TODO: should try to lookup for an existing map with a compatible parameters
                        // TODO: maybe "hard-attach" maps to lights so that very customized shaders
                        //       can be generated (i.e. shaders without branching)
                        MINIRE_WARNING("cube shadow map have to be rebuild");
                        _cubeShadowMaps[shadowMapIndex] =
                            std::make_shared<rasterizer::CubeShadowMap>(*shadowParams);
                    }

                    shadowMap = _cubeShadowMaps[shadowMapIndex];

                    shadowMapIndex++;
                }

                result.emplace_back(rasterizer::CulledPointLight
                {
                    ._position = position,
                    ._color = color,
                    ._attenuation = attenuation,
                    ._shadowMap = shadowMap,
                    ._shadowMapFarPlane = 0,
                });
            });
        return result;
    }

    rasterizer::CulledPrimitives
    Rasterizer::cullPrimitives(SceneImpl const & scene)
    {
        rasterizer::CulledPrimitives result;
        // TODO: result.reserve
        scene.cullModels(
            [&result] (rasterizer::Mesh const & mesh,
                       glm::vec3 const & emissiveFactor,
                       glm::mat4 const & transform,
                       material::SkinningVector && skinningVector,
                       size_t const obpId)
            {
                for(size_t i = 0; i < mesh.primitives(); ++i)
                {
                    result.emplace_back(rasterizer::CulledPrimitive
                    {
                        ._mesh = mesh,
                        ._primitiveIndex = i,
                        ._emissiveFactor = emissiveFactor,
                        ._transform = transform,
                        ._skinningVector = std::move(skinningVector),
                        ._obpId = obpId,
                    });
                }
            });
        return result;
    }

    void Rasterizer::draw(SceneImpl const & scene)
    {
        auto directionalLights = cullDirectionalLights(scene);
        auto pointLights = cullPointLights(scene);
        auto primitives = cullPrimitives(scene);
        shadowPass(scene, primitives, directionalLights, pointLights);
        // TODO: color pass should also use "primitives"
        colorPass(scene, directionalLights, pointLights);
        draw2d();
    }

    void Rasterizer::shadowPass(SceneImpl const & scene,
                                rasterizer::CulledPrimitives const & culledPrimitives,
                                rasterizer::CulledDirectionalLights & culledDirectionalLights,
                                rasterizer::CulledPointLights & culledPointLights)
    {
        // TODO: maybe do min/max w/ scene AABB?
        utils::ViewFrustum const & viewFrustum = scene.viewpoint().viewFrustum();

        // build shadow maps for directional lights (if any)
        for(rasterizer::CulledDirectionalLight & light : culledDirectionalLights)
        {
            if (light._shadowMap)
            {
                light._viewProjection = light._shadowMap->perform(
                    culledPrimitives, light._position, light._direction,
                    viewFrustum);
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
                    culledPrimitives, light._position, viewFrustum);
            }
            else
            {
                light._shadowMapFarPlane = 0;
            }
        }
    }

    void Rasterizer::colorPass(SceneImpl const & scene,
                               rasterizer::CulledDirectionalLights const & culledDirectionalLights,
                               rasterizer::CulledPointLights const & culledPointLights)
    {
        // setup Primary FBO
        _primaryFbo.bind();

        // initial setup
        assert(_screenWidth != 0);
        assert(_screenHeight != 0);
        MINIRE_GL(glViewport, 0, 0, _screenWidth, _screenHeight);
        MINIRE_GL(glClearColor, 0.0f, 0.2f, 0.2f, 1.0f); // TODO: into parameters
        MINIRE_GL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // clean up ID buffer
        GLuint clearId = 0;
        MINIRE_GL(glClearBufferuiv, GL_COLOR, 1, &clearId);

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

        // Queue PBO for a hot pixel
        assert(_hotFragmentPbo);
        _primaryFbo.bind(GL_READ_FRAMEBUFFER);
        MINIRE_GL(glReadBuffer, GL_COLOR_ATTACHMENT1);
        _hotFragmentPbo->bind();
        MINIRE_GL(glReadPixels, _hotFragmentX, _screenHeight - 1 - _hotFragmentY, 1, 1,
                  GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        _hotFragmentPbo->unbind();
        MINIRE_GL(glBindFramebuffer, GL_READ_FRAMEBUFFER, 0);

        // Prepare the fullscreen pass
        _primaryFbo.unbind();   // will bind to 0
        MINIRE_GL(glViewport, 0, 0, _screenWidth, _screenHeight);
        MINIRE_GL(glDisable, GL_DEPTH_TEST);
        MINIRE_GL(glDisable, GL_CULL_FACE);
        MINIRE_GL(glClear, GL_COLOR_BUFFER_BIT);

        // activate a progrma
        _screenQuadProgram.use();

        // setup textures and uniform
        assert(_colorBuffer);
        MINIRE_GL(glActiveTexture, GL_TEXTURE0);
        _screenQuadProgram.setUniform(_screenTextureUniform, GLint(0));
        _colorBuffer->bind();

        assert(_idBuffer);
        MINIRE_GL(glActiveTexture, GL_TEXTURE1);
        _screenQuadProgram.setUniform(_idTextureUniform, GLint(1));
        _idBuffer->bind();

        // setup screen-pass UBO
        MINIRE_INVARIANT(scene.pixelEdgeOutlines().size() <= kMaxPixelOutlines,
                         "too many pixel-outline objects ({}), while limit is {}",
                         scene.pixelEdgeOutlines().size(), kMaxPixelOutlines);
        assert(_screenPassUbo);
        ScreenPassDatablock & datablock = _screenPassUbo->datablock();
        datablock._pixelOutlineCount = 0;
        for(auto const & [opbId, pixelEdge] : scene.pixelEdgeOutlines())
        {
            assert(datablock._pixelOutlineCount < kMaxPixelOutlines);
            PixelOutline & pixelOutline = datablock._pixelOutlines[datablock._pixelOutlineCount];

            assert(opbId > 0);
            assert(opbId <= std::numeric_limits<uint32_t>::max());

            pixelOutline._id = static_cast<uint32_t>(opbId);
            pixelOutline._color = pixelEdge._color;

            datablock._pixelOutlineCount++;
        }
        _screenPassUbo->update();

        // run the pass
        assert(_screenQuadVao);
        _screenQuadVao->bind();
        MINIRE_GL(glDisable, GL_BLEND);
        MINIRE_GL(glDrawArrays, GL_TRIANGLES, 0, 6);
    }

    void Rasterizer::draw3d(SceneImpl const & scene,
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
        MINIRE_GL(glEnable, GL_BLEND);
        MINIRE_GL(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
                return a->effectiveZOrder() < b->effectiveZOrder();
            });

        for(rasterizer::Drawable * drawable : _drawables)
        {
            assert(drawable);
            drawable->draw(_2dProjection);
        }
    }
}
