#pragma once

#include <minire/content/id.hpp>
#include <minire/content/path.hpp>
#include <minire/events/application.hpp>
#include <minire/events/controller.hpp>
#include <minire/utils/aabb.hpp>
#include <minire/utils/barrier.hpp>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace minire::content { class Lease; }
namespace minire::content { class Manager; }
namespace minire::gui { class Component; }
namespace minire::models { class FontFace; }
namespace minire::text { class FormattedString; }

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

        explicit BasicController(content::Manager &, size_t const maxFps);

        virtual ~BasicController();

        // called from application thread (i.e. rendering thread)
        void run(events::application::OnResize const & initial);

        void shutdown(); // NOTE: MUST be called from application thread
                         //       just before calling dtor.
                         //       Can't put this into base dtor because she is virtual
                         //       and derived class will destroy their data while
                         //       controller's worker thread is still running

    public: // NOTE: these calls are thread-safe
        BatchQueue pull();

        void push(events::ApplicationQueue &&);

        void quit();

        std::unique_ptr<content::Lease> borrow(content::Id const &) const;

        glm::vec2 measure(text::FormattedString const &,
                          content::Id const &) const;

        glm::vec2 measure(text::FormattedString const &,
                          models::FontFace const &) const;

        utils::Aabb measure(content::Path const &) const;

        /**
         * When low lantecy input mode is enabled, messages from an Application
         * will be delivered as soon as possible. As a side effect, Controller's
         * FPS will sometimes be higher than \a maxFps (during an intense input stream).
         *
         * In addition, this mode might disrupt consistency of Application's FPS>
         * */
        void setLowLatencyInput(bool enabled);

        bool lowLatencyInput() const;

    protected:
        void enqueueRaw(events::Controller &&);

        template<typename EventType,
                 typename... Args>
        void enqueue(Args && ... args)
        {
            enqueueRaw(EventType(std::forward<Args>(args)...));
        }

        double frameTime() const { return _frameTime; }

        double absoluteTime() const { return _absoluteTime; }

    protected:
        // called from controller thread
        virtual void start();
        virtual void step();
        virtual void finish();

    protected:
        // NOTE: input events are returning boolean value that
        //       indicates a consumation of an event.
        //       Normally, consumed events shouldn't be processed
        //       by a descendant classes. The following pattern is
        //       recommended (usually make sense w/ GuiController):
        //
        //       class Foo : public BasicController // or GuiController
        //       {
        //           bool handle(OnMouseDown const & e) override
        //           {
        //               if (BasicController::handle(e))
        //                   return true;
        //               ...
        //               return true;
        //           }
        //       };
        //
        //       It is also legal to not call Base class's handle in
        //       situations when descedant intend to capture inputs
        //       exclusively (for example, when moving a camera by a
        //       mouse and need to prevent activation of GUI elements).

        virtual void handle(events::application::OnFps const &);
        virtual void handle(events::application::OnResize const &);
        virtual bool handle(events::application::OnMouseWheel const &);
        virtual bool handle(events::application::OnMouseMove const &);
        virtual bool handle(events::application::OnMouseDown const &);
        virtual bool handle(events::application::OnMouseUp const &);
        virtual bool handle(events::application::OnKeyUp const &);
        virtual bool handle(events::application::OnKeyDown const &);
        virtual bool handle(events::application::OnTextInput const &);
        virtual void handle(events::application::OnRayCaster const &);

        virtual void postprocess();

    private:
        void worker(events::application::OnResize const & initial);
        void handle(events::ApplicationQueue const &);
        void finishCurrentBatch(double);

    private:
        content::Manager       & _contentManager;
        size_t const             _maxFps = 0;

        std::mutex               _applicationEventsMutex;
        std::condition_variable  _applicationEventsCond;
        events::ApplicationQueue _applicationEvents;

        std::mutex               _pendedControllerEventsMutex;
        BatchQueue               _pendedControllerEvents;
        Batch                    _currentEventsBatch;

        std::atomic<bool>        _working;
        std::atomic<bool>        _quitRequest;
        std::atomic<bool>        _lowLatencyInput;
        std::thread              _thread;
        utils::Barrier           _initBarrier;
        double                   _frameTime = 0.0;
        double                   _absoluteTime = 0.0; // seconds since begin

#       ifndef NDEBUG
        std::vector<size_t>      _eventsLatency;
#       endif

        // This helps to open access to enqueue() without
        // making it public.
        friend class gui::Component;
    };
}
