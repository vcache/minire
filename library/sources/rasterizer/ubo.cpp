#include <rasterizer/ubo.hpp>

#include <minire/models/shadow-params.hpp>
#include <minire/utils/overloaded.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <rasterizer/binding-points.hpp>
#include <rasterizer/cube-shadow-map.hpp>
#include <rasterizer/flat-shadow-map.hpp>
#include <rasterizer/shadow-params-helper.hpp>

#include <algorithm>
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

    void Ubo::bindBufferRange(opengl::Program const & program,
                              std::string const & uniformName) const
    {
        GLuint blockIndex = program.getUniformBlockIndex(uniformName.c_str());
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
            _datablock._pointLightsCount = 0;
            size_t const count = std::min(culledPointLights.size(), maxPointLights());
            for(size_t i = 0; i < count; ++i)
            {
                CulledPointLight const & light = culledPointLights[i];
                auto & dst = _datablock._pointLights[_datablock._pointLightsCount];
                dst._position = glm::vec4(light._position, 1.0);
                dst._color = light._color;
                dst._attenuation = light._attenuation;
                dst._shadowMapFarPlane = light._shadowMapFarPlane;
                dst._shadowMapIndex = static_cast<uint32_t>(i);
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
