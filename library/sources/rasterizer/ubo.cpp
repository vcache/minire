#include <rasterizer/ubo.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <scene.hpp>

#include <cassert>

namespace minire::rasterizer
{
    static const GLuint kUboBindingPoint = 10; // TODO: why the fuck not 0?

    Ubo::Ubo()
    {
        _glUbo.bindBufferBase(kUboBindingPoint);
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
        // TODO: "10" binding harcoded
        GLuint blockIndex = program.getUniformBlockIndex("BznkDatablock");
        MINIRE_GL(glUniformBlockBinding,
                  program.id(),
                  blockIndex,
                  kUboBindingPoint);
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
                        Scene const & scene)
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
                dst._hasShadows = light._shadowMap.operator bool();
                _datablock._directionalLightsCount++;
            }
        }

        _datablock._pointLightsCount = scene.cullPointLights(
            maxPointLights(),
            [this](size_t index,
                   glm::vec3 const & position,
                   glm::vec4 const & color,
                   glm::vec4 const & attenuation)
            {
                assert(index < maxPointLights());
                auto & dst = _datablock._pointLights[index];
                dst._position = glm::vec4(position, 1.0);
                dst._color = color;
                dst._attenuation = attenuation;
            });

        _invalidated = true; // TODO: do only when changed
    }
}
