#pragma once

#include <minire/instrumentation/histogram.hpp>

#include <cassert>
#include <chrono>
#include <memory>
#include <string>

namespace minire::instrumentation
{
    // NOTE: Stopwatch must NOT outlive Timekeeper
    // TODO: eliminate this class in Release-builds (?)
    template<typename T = double>
    class Stopwatch
    {
    public:
        explicit Stopwatch(std::string const & name)
            : _name(name)
            , _begin(std::chrono::steady_clock::now())
        {}

        explicit Stopwatch(std::string const & name,
                           std::shared_ptr<Histogram<T>> timekeeper)
            : _name(name)
            , _timekeeper(timekeeper)
            , _begin(std::chrono::steady_clock::now())
        {}

        ~Stopwatch()
        {
            if (_timekeeper)
            {
                T const time = lap();
                _timekeeper->collectMeasurement(_name, time);
            }
        }

        // seconds by default
        template<typename R = std::ratio<1>>
        T lap()
        {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<T, R> elapsed = now - _begin;
            _begin = now;
            return elapsed.count();
        }

    private:
        using Timepoint = std::chrono::time_point<std::chrono::steady_clock>;

        std::string const             _name;
        std::shared_ptr<Histogram<T>> _timekeeper;
        Timepoint                     _begin;
    };
}
