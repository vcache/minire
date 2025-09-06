#pragma once

#include <minire/events/application/base.hpp>

#include <cstddef>

namespace minire::events::application
{
    /**
     * Is sent from an application when FPS statistics is updated.
     * */
    struct OnFps : public Base
    {
        static models::QueryEventFilter constexpr kQueueEventFilter = models::QueryEventFilter::kOnFps;

        size_t _fps;        // Frames per second
        double _mft;        // Mean frame time
        size_t _frame;      // Current frame number

        template<typename... BaseArgs>
        OnFps(size_t fps, double mft, size_t frame,
              BaseArgs && ... baseArgs)
            : Base(std::forward<BaseArgs>(baseArgs)...)
            , _fps(fps)
            , _mft(mft)
            , _frame(frame)
        {}
    };
}
