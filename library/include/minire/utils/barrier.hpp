#pragma once

#include <mutex>
#include <condition_variable>

namespace minire::utils
{
    class Barrier
    {
    public:
        void wait()
        {
            std::unique_lock<std::mutex> lck(_mutex);
            _cv.wait(lck, [this]{ return _ready; });
        }

        void notify()
        {
            std::unique_lock<std::mutex> lck(_mutex);
            _ready = true;
            _cv.notify_all();
        }

        void reset()
        {
            std::unique_lock<std::mutex> lck(_mutex);
            _ready = false;
        }

    private:
        std::mutex              _mutex;
        std::condition_variable _cv;
        bool                    _ready = false;
    };
}
