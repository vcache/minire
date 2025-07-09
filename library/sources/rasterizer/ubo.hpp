#pragma once

#include <rasterizer/ubo/datablock.hpp>
#include <scene/point-light.hpp>

#include <opengl/ubo.hpp>

namespace minire::opengl { class Program; }

namespace minire::rasterizer
{
    class Ubo
    {
    public:
        Ubo();

        void bind();

        void bindBufferRange(opengl::Program &) const;

        static std::string interfaceBlock();

        static size_t maxLights() { return ubo::Datablock::kMaxLights; }

    public:
        void setViewProjection(glm::mat4 const &, size_t);

        void setViewPosition(glm::vec4 const &);

        void setLights(scene::PointLightRef::List const &);

    private:
        using GlUbo = opengl::UBO<ubo::Datablock>;

        GlUbo          _glUbo;
        ubo::Datablock _datablock;
        size_t         _viewProjectionVersion = -1;
        bool           _invalidated = true;
    };
}
