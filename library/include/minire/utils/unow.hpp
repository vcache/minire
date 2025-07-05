#pragma once

#include <chrono>

namespace minire::utils
{
    static inline size_t uNow()
    {
        using namespace std::chrono;
        time_point<steady_clock> now(steady_clock::now());
        auto duration = now.time_since_epoch();
        return duration_cast<microseconds>(duration).count();
    }
}
