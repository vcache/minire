#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace minire::utils
{
    // TODO: should be covered with tests
    // TODO: move into sources/ dir
    template<typename Element,
             bool kBarrierWrite>
    class SafeQueue final
    {
    public:
        using Store = std::vector<Element>;

        SafeQueue()
            : _stores{Store(), Store()}
            , _mutex()
            , _cv()
            , _primary(0)
            , _readReady(false)
            , _writeReady(true)
        {}

    // writer routines //

    public:
        void push(Element && element)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            assert(!kBarrierWrite || _writeReady);
            writing().push_back(std::forward<Element>(element));
        }

        template<typename... Args>
        void emplace(Args && ... args)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            assert(!kBarrierWrite || _writeReady);
            writing().emplace_back(std::forward<Args>(args)...);
        }

        template<typename Fn>
        void apply(Fn fn)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            assert(!kBarrierWrite || _writeReady);
            for(auto const & i : writing()) fn(i);
        }

        template<typename Fn>
        void remove_if(Fn fn)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            assert(!kBarrierWrite || _writeReady);
            auto & store = writing();
            auto end = std::remove_if(store.begin(), store.end(), fn);
            store.resize(std::distance(store.begin(), end));
        }

        bool finish()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _readReady = true;
            return writing().empty();
        }

        void waitWriteReady()
        {
            if constexpr (kBarrierWrite) 
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this]{ return _writeReady; });
            }
        }

    // reader routines //

    public:
        bool ready() const
        {
            std::unique_lock<std::mutex> lock(_mutex);
            return !writing().empty() && _readReady;
        }

        Store const & swap() const
        {
            static const Store kEmpty;

            {
                // lock the mutex and check if read could be done
                std::unique_lock<std::mutex> lock(_mutex);
                if (!_readReady) return kEmpty;

                // switch buffers
                _primary = (_primary + 1) % 2;
                writing().clear();
                _readReady = false;

                // notify writer that reading happened
                if constexpr (kBarrierWrite)
                {
                    _writeReady = true;
                    _cv.notify_one();
                }
            }

            // since only a (single) reader (should) call swap()
            // it is safe to return ref to secondary without any locks
            return reading();
        }

    private:
        Store & writing() const
        {
            return _stores[_primary];
        }

        Store & reading() const
        {
            return _stores[1 - _primary];
        }
 
    private:
        // TODO: why they all are mutable? How drunk was I?
        mutable std::array<Store, 2>    _stores;
        mutable std::mutex              _mutex;
        mutable std::condition_variable _cv;
        mutable int                     _primary;
        mutable bool                    _readReady;
        mutable bool                    _writeReady;
    };
}
