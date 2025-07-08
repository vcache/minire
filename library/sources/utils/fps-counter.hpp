#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <utility>

namespace minire::utils
{
    class FpsCounter
    {
    public:
        explicit FpsCounter(size_t updateInterval = 5)
            : _updateInterval(updateInterval)
            , _fpsUpdateInterval(std::chrono::duration<size_t>(updateInterval))
            , _FPSCount(0)
            , _FPSReper()
            , _meanFrameTimePrev()
            , _meanFrameTimeValue(0.0)
            , _meanFrameTimeCount(0)
        {}

        std::optional<std::pair<size_t, double>> registerFrame()
        {
            using namespace std::chrono;

            auto now = system_clock::now();
            auto spent = now - _FPSReper;

            ++_meanFrameTimeCount;
            double deltaMs = (now - _meanFrameTimePrev).count() / 1000000.0;
            _meanFrameTimeValue += deltaMs;
            _meanFrameTimePrev = now;

            std::optional<std::pair<size_t, double>> result;

            if (spent > _fpsUpdateInterval)
            {
                size_t resultFps = _FPSCount / _updateInterval;
                _FPSCount = 0;
                _FPSReper = now;

                double resultMft = _meanFrameTimeValue / static_cast<double>(_meanFrameTimeCount);
                _meanFrameTimeValue = 0.0;
                _meanFrameTimeCount = 0;

                result.emplace(resultFps, resultMft);
            }

            ++_FPSCount;

            return result;
        }
    private:
        size_t                                _updateInterval;
        std::chrono::system_clock::duration   _fpsUpdateInterval;
        size_t                                _FPSCount;
        std::chrono::system_clock::time_point _FPSReper;

        std::chrono::system_clock::time_point _meanFrameTimePrev;
        double                                _meanFrameTimeValue;
        size_t                                _meanFrameTimeCount;
    };
}
