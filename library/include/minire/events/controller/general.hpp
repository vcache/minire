#pragma once

#include <cstddef>
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
}
