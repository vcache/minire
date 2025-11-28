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
    void Ubo::setLights(Scene const & scene)
    {
        _datablock._directionalLightsCount = scene.cullDirectionalLights(
            maxDirectionalLights(),
            [this](size_t index,
                   glm::vec3 const & direction,
                   glm::vec3 const & color)
            {
                assert(index < maxDirectionalLights());
                auto & dst = _datablock._directionalLights[index];
                dst._direction = direction;
                dst._color = color;
            });

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
