#include <gpu/render/ubo.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>

#include <algorithm> // for std::min
#include <cassert>

namespace minire::gpu::render
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

    void Ubo::bindBufferRange(opengl::Program & program) const
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
    void Ubo::setLights(scene::PointLightRef::List const & pointLights)
    {
        assert(pointLights.size() <= maxLights());

        _datablock._lightsCount = std::min(pointLights.size(),
                                           maxLights());
        for(size_t i(0); i < _datablock._lightsCount; ++i)
        {
            auto const & src = pointLights[i];
            auto & dst = _datablock._pointLights[i];

            dst._position = src.get()._origin;
            dst._color = src.get()._color;
            dst._attenuation = src.get()._attenuation;
        }

        _invalidated = true; // TODO: do only when changed
    }
}
