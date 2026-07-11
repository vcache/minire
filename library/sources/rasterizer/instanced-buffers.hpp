#pragma once

#include <opengl/ssbo.hpp>
#include <opengl/vbo.hpp>
#include <rasterizer/fenced-data.hpp>

namespace minire::rasterizer
{
    struct InstancedBuffers
    {
        opengl::VBO  _vbo;
        opengl::SSBO _ssbo;

        InstancedBuffers()
            : _vbo(GL_ARRAY_BUFFER)
            , _ssbo()
        {}
    };

    using InstancedBuffersPool = FencedDataPool<InstancedBuffers>;
}