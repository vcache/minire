#include <rasterizer/ubo.hpp>

#include <minire/models/shadow-params.hpp>
#include <minire/utils/overloaded.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <rasterizer/binding-points.hpp>
#include <rasterizer/cube-shadow-map.hpp>
#include <rasterizer/flat-shadow-map.hpp>

#include <cassert>

namespace minire::rasterizer
{
    Ubo::Ubo()
    {
        _glUbo.bindBufferBase(kModelsBindingPoint);
    }

    void Ubo::bind()
    {
        if (_invalidated)
        {
            _glUbo.update(_datablock); // will also bind()
            _invalidated = false;
        }
        else
        {
            _glUbo.bind();
        }
    }

    void Ubo::bindBufferRange(opengl::Program const & program) const
    {
        // TODO: harcoded "BznkDatablock"
        GLuint blockIndex = program.getUniformBlockIndex("BznkDatablock");
        MINIRE_GL(glUniformBlockBinding,
                  program.id(),
                  blockIndex,
                  kModelsBindingPoint);
    }

    std::string Ubo::interfaceBlock()
    {
        return ubo::makeInterfaceBlock<ubo::Datablock>();
    }

    void Ubo::setViewProjection(glm::mat4 const & v,
                                size_t version)
    {
        if (_viewProjectionVersion != version)
        {
            _datablock._viewProjection = v;
            _viewProjectionVersion = version;
            _invalidated = true;
        }
    }

    void Ubo::setViewPosition(glm::vec4 const & v)
    {
        if (_datablock._viewPosition != v)
        {
            _datablock._viewPosition = v;
            _invalidated = true;
        }
    }

    namespace
    {
        template<typename LightDatablock>
        void setupShadowParams(models::ShadowParams const & shadowParams,
                               LightDatablock & dst)
        {
            using namespace ::minire::models::shadow_params;

            // setup shadow method
            dst._method = std::visit(utils::Overloaded
            {
                [&shadowParams](method::Standard const &)
                {
                    assert(std::holds_alternative<std::monostate>(shadowParams._filter) ||
                           std::holds_alternative<filter::PCF>(shadowParams._filter));
                    bool const pcf = std::holds_alternative<filter::PCF>(shadowParams._filter);
                    return pcf ? 2 : 1;
                },
                [&dst](method::ESM const & esm)
                {
                    dst._methodArg = esm._factor;
                    return 3;
                },
                [&dst](method::LogESM const & logEsm)
                {
                    dst._methodArg = logEsm._factor;
                    return 4;
                },
            }, shadowParams._method);

            // setup normal bias
            std::visit(utils::Overloaded
            {
                [&dst](bias::Constant const & bias)
                {
                    dst._normalBiasMode = 0;
                    dst._normalBiasBase = bias._biasBase;
                    dst._normalBiasMax = bias._biasBase;
                },
                [&dst](bias::SlopScaled const & bias)
                {
                    dst._normalBiasMode = 1;
                    dst._normalBiasBase = bias._biasBase;
                    dst._normalBiasMax = bias._maxBias;
                },
            }, shadowParams._normalBias);

            // setup depth bias
            std::visit(utils::Overloaded
            {
                [&dst](bias::Constant const & bias)
                {
                    dst._depthBiasMode = 0;
                    dst._depthBiasBase = bias._biasBase;
                    dst._depthBiasMax = bias._biasBase;
                },
                [&dst](bias::SlopScaled const & bias)
                {
                    dst._depthBiasMode = 1;
                    dst._depthBiasBase = bias._biasBase;
                    dst._depthBiasMax = bias._maxBias;
                },
            }, shadowParams._depthBias);

            // setup smoothing parameters
            dst._smoothStepLeft = shadowParams._smoothStep.first;
            dst._smoothStepRight = shadowParams._smoothStep.second;
        }

        template<typename LightDatablock>
        void dropShadowParams(LightDatablock & dst)
        {
            dst._method = 0;
            dst._normalBiasMode = 0;
            dst._normalBiasBase = 0;
            dst._normalBiasMax = 0;
            dst._depthBiasMode = 0;
            dst._depthBiasBase = 0;
            dst._depthBiasMax = 0;
            dst._smoothStepLeft = 0;
            dst._smoothStepRight = 0;
        }
    }

    // TODO: try to minimize changes (esp. when nothing changed)
    void Ubo::setLights(CulledDirectionalLights const & culledDirectionalLights,
                        CulledPointLights const & culledPointLights)
    {
        {
            assert(culledDirectionalLights.size() <= maxDirectionalLights());
            _datablock._directionalLightsCount = 0;
            for(CulledDirectionalLight const & light : culledDirectionalLights)
            {
                auto & dst = _datablock._directionalLights[_datablock._directionalLightsCount];
                dst._direction = light._direction;
                dst._color = light._color;
                dst._viewProjection = light._viewProjection;
                if (light._shadowMap)
                {
                    setupShadowParams(light._shadowMap->shadowParams(), dst);
                }
                else
                {
                    dropShadowParams(dst);
                }

                _datablock._directionalLightsCount++;
            }
        }

        {
            assert(culledPointLights.size() <= maxPointLights());
            _datablock._pointLightsCount = 0;
            for(CulledPointLight const & light : culledPointLights)
            {
                auto & dst = _datablock._pointLights[_datablock._pointLightsCount];
                dst._position = glm::vec4(light._position, 1.0);
                dst._color = light._color;
                dst._attenuation = light._attenuation;
                dst._shadowMapFarPlane = light._shadowMapFarPlane;
                if (light._shadowMap)
                {
                    setupShadowParams(light._shadowMap->shadowParams(), dst);
                }
                else
                {
                    dropShadowParams(dst);
                }

                _datablock._pointLightsCount++;
            }
        }

        _invalidated = true; // TODO: do only when changed
    }
}
