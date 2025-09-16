#pragma once

#include <minire/content/id.hpp>
#include <minire/models/vertex-buffer.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace minire::events::controller
{
    struct Quit
    {};

    struct MouseGrab
    {
        bool _grab;
    };

    struct DebugDrawsUpdate
    {
        std::vector<float> _linesBuffer;
    };

    struct SetInstrumentation
    {
        bool _enabled;
    };

    struct NewResourceLayer
    {
        std::string _name;
    };

    struct DisposeResourceLayer
    {
        std::string _name;
    };

    struct ContentManagerCleanup
    {
        bool _force;
    };

    // Low-level rasterizer API

    // Note that this command won't perform inter-frame lerping,
    // therefore, animated meshes might appear jerky if Controller's FPS
    // is lower that Raterizer FPS.
    struct CreateVertexBuffer
    {
        content::Id          _id;               // The VertexBuffer will be available by a path:
                                                //  content::path::Special::kVertexBuffers/{_id}
        models::VertexBuffer _vertexBuffer;     // Controller MUST NOT modify provided buffers,
                                                // otherwise thread-safety will be broken.
        bool                 _override;         // If true, existing one will be rewritten,
                                                // otherwise, a runtime-error will be generated
    };
}
