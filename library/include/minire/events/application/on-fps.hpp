#pragma once

#include <cstddef>

namespace minire::events::application
{
    /**
     * Is sent from an application when FPS statistics is updated.
     * */
    struct OnFps
    {
        size_t _fps;        // Frames per second
        double _mft;        // Mean frame time
        size_t _frame;      // Current frame number

        OnFps(size_t fps, double mft, size_t frame)
            : _fps(fps)
            , _mft(mft)
            , _frame(frame)
        {}
    };
}
