#pragma once

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
}
