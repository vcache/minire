#pragma once

#include <rasterizer/ubo/datablock.hpp>

#include <opengl/ubo.hpp>

namespace minire { class Scene; }

namespace minire::opengl { class Program; }

namespace minire::rasterizer
{
    class Ubo
    {
    public:
        Ubo();

        void bind();

        void bindBufferRange(opengl::Program const &) const;

        static std::string interfaceBlock();

        static size_t maxDirectionalLights() { return ubo::Datablock::kMaxDirectionalLights; }

        static size_t maxPointLights() { return ubo::Datablock::kMaxPointLights; }

    public:
        void setViewProjection(glm::mat4 const &, size_t);

        void setViewPosition(glm::vec4 const &);

        void setLights(Scene const &);

    private:
        using GlUbo = opengl::UBO<ubo::Datablock>;

        GlUbo          _glUbo;
        ubo::Datablock _datablock;
        size_t         _viewProjectionVersion = -1;
        bool           _invalidated = true;
    };
}
