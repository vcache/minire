#pragma once

#include <chrono>
#include <cstddef>
#include <thread>

namespace minire::utils
{
    class FpsLimiter
    {
    public:
        // NOTE: 0 = no limit
        explicit FpsLimiter(size_t maxFps = 0)
        {
            setMaxFps(maxFps);
        }

        void setMaxFps(size_t maxFps)
        {
            using namespace std::chrono;

            _maxFps = maxFps;

            if (_maxFps != 0)
            {
                _frameDuration = duration_cast<steady_clock::duration>(
                    duration<double>(1.0 / static_cast<double>(_maxFps))
                );
            }
            else
            {
                _frameDuration = steady_clock::duration{};
            }
        }

        void maybeSleep()
        {
            if (_maxFps == 0)
                return;

            auto now = std::chrono::steady_clock::now();
            auto targetTime = _lastTimePoint + _frameDuration;

            if (now < targetTime)
            {
                std::this_thread::sleep_until(targetTime);
                _lastTimePoint = targetTime;
            }
            else
            {
                _lastTimePoint = now;
            }
        }

    private:
        size_t                                _maxFps = 0;
        std::chrono::steady_clock::time_point _lastTimePoint{};
        std::chrono::steady_clock::duration   _frameDuration{};
    };
}