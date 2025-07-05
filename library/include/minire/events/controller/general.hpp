#pragma once

#include <cstddef>
#include <vector>

namespace minire::events::controller
{
    struct NewEpoch
    {
        size_t _epochNumber;
        double _epochLength;
    };

    struct MouseGrab
    {
        bool _grab;
    };

    struct DebugDrawsUpdate
    {
        std::vector<float> _linesBuffer;
    };

    struct Quit
    {};
}
