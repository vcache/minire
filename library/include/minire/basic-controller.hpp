#pragma once

#include <minire/events/application.hpp>
#include <minire/events/controller.hpp>
#include <minire/utils/barrier.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

namespace minire
{
    // NOTE: DO NOT init or deinit derived classes from ctor/dtor
    //       (especially when working w/ queue()), use start()/finish() instead
    class BasicController
    {
    public:
        // TODO: these should be hidden from public interfaces
        // TODO: maybe use std::list for faster pop_front() in the Application
        struct Batch
        {
            std::vector<events::Controller> _events;
            double                          _duration = 0;
        };

        using BatchQueue = std::vector<Batch>;

    public:
        using Uptr = std::unique_ptr<BasicController>;

        explicit BasicController(size_t const maxFps);

        virtual ~BasicController();

        // called from application thread (i.e. rendering thread)
        void run(events::application::OnResize const & initial);

        void shutdown(); // NOTE: MUST be called from application thread
                         //       just before calling dtor.
                         //       Can't put this into base dtor because she is virtual
                         //       and derived class will destroy their data while
                         //       controller's worker thread is still running

    public:
        // NOTE: this call is thread-safe
        BatchQueue pull();

        // NOTE: this call is thread-safe
        void push(events::ApplicationQueue &&);

        // NOTE: this call is thread-safe
        void quit();

    protected:
        template<typename EventType,
                 typename... Args>
        void enqueue(Args && ... args)
        {
            _currentEventsBatch._events.emplace_back(
                EventType(std::forward<Args>(args)...));
        }

        double frameTime() const { return _frameTime; }

        double absoluteTime() const { return _absoluteTime; }

    protected:
        // called from controller thread
        virtual void start();
        virtual void step();
        virtual void finish();

    protected:
        virtual void handle(events::application::OnFps const &);
        virtual void handle(events::application::OnResize const &);
        virtual void handle(events::application::OnMouseWheel const &);
        virtual void handle(events::application::OnMouseMove const &);
        virtual void handle(events::application::OnMouseDown const &);
        virtual void handle(events::application::OnMouseUp const &);
        virtual void handle(events::application::OnKeyUp const &);
        virtual void handle(events::application::OnKeyDown const &);
        virtual void handle(events::application::OnTextInput const &);

        virtual void postprocess();

    private:
        void worker(events::application::OnResize const & initial);
        void handle(events::ApplicationQueue const &);
        void finishCurrentBatch(double);

    private:
        size_t const             _maxFps = 0;

        std::mutex               _applicationEventsMutex;
        events::ApplicationQueue _applicationEvents;

        std::mutex               _pendedControllerEventsMutex;
        BatchQueue               _pendedControllerEvents;
        Batch                    _currentEventsBatch;

        std::atomic<bool>        _working;
        std::atomic<bool>        _quitRequest;
        std::thread              _thread;
        utils::Barrier           _initBarrier;
        double                   _frameTime = 0.0;
        double                   _absoluteTime = 0.0; // seconds since begin
    };
}
