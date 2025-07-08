#include <minire/basic-controller.hpp>

#include <minire/errors.hpp>
#include <minire/utils/unow.hpp>

#include <cassert>
#include <variant>

namespace minire
{
    BasicController::BasicController(events::ApplicationQueue const & applicationEvents)
        : _applicationEvents(applicationEvents)
        , _working(true)
        , _quitRequest(false)
    {}

    BasicController::~BasicController()
    {
        assert(!_working);
        assert(!_thread.joinable());
    }

    void BasicController::shutdown()
    {
        _working = false;
        _thread.join();
    }
    
    void BasicController::run(events::application::OnResize const & initial,
                              size_t const maxFps)
    {
        _thread = std::thread(
            [this, maxFps, &initial]
            {
                try
                {
                    worker(maxFps, initial);
                }
                catch(Exception const & e)
                {
                    _initBarrier.notify();
                    MINIRE_THROW("controller startup failure: {}", e.what());
                }
                catch(std::exception const & e)
                {
                    _initBarrier.notify();
                    MINIRE_THROW("controller startup failure: {}", e.what());
                }
                catch(...)
                {
                    _initBarrier.notify();
                    MINIRE_THROW("controller startup failure: (unknown exception)");
                }
            });

        // wait until the worker thread finishes its initialization routines
        _initBarrier.wait();
    }

    void BasicController::worker(size_t const maxFps,
                                 events::application::OnResize const & initial)
    {
        static const std::chrono::microseconds kSleepDuration(10);

        MINIRE_INVARIANT(initial._width > 0 && initial._height > 0,
                         "bad initial screen size: {}x{}", initial._width, initial._height);

        enqueue<events::controller::NewEpoch>(0, 0);
        start();
        handle(initial);
        _controllerEvents.finish();
        _initBarrier.notify();

        size_t frameBegin = utils::uNow(), frameEnd; // microseconds
        size_t const minIteration = size_t(1e6 / static_cast<double>(maxFps));
        double absoluteTime = 0; // seconds
        _frameTime = static_cast<double>(minIteration) / 1e6;
        _frameNum = 1; // 0-th epoch has already happened (see NewEpoch above)

        while(_working)
        {
            // if a reader is too slow, we should wait
            // until the writing queue to be cleared
            _controllerEvents.waitWriteReady(); // TODO why?? it break the whole idea of double-buffering
                                                //      (maybe to avoid controler iterations skips?)

            enqueue<events::controller::NewEpoch>(_frameNum, _frameTime);

            // handle input events
            handle(_applicationEvents.swap());

            // do a logic step
            step();

            // do a post-processing step
            postprocess();

            if (_quitRequest)
            {
                _working = false;
                enqueue<events::controller::Quit>();
                _controllerEvents.finish();
                continue;
            }

            // signal render thread about logic frame finish
            _controllerEvents.finish();

            // wait until maxFps reached
            while (frameBegin + minIteration > utils::uNow())
            {
                std::this_thread::sleep_for(kSleepDuration);
                if (_applicationEvents.ready()) break;
            }

            // collect frame statistics
            frameEnd = utils::uNow();
            _frameTime = double(frameEnd - frameBegin) / 1e6;
            _frameTime = std::min(1.0, _frameTime); // prevent from going haywire
            absoluteTime += _frameTime;
            ++_frameNum;

            // prepare next frame
            frameBegin = frameEnd;
        }

        finish();
        _controllerEvents.finish();
    }

    void BasicController::quit()
    {
        _quitRequest = true;
    }

    void BasicController::handle(events::ApplicationQueue::Store const & events)
    {
        for(auto const & event: events)
        {
            std::visit([this](auto const & e) { handle(e); }, event);
        }
    }

    void BasicController::start() {}

    void BasicController::step() {}

    void BasicController::postprocess() {}

    void BasicController::finish() {}

    void BasicController::handle(events::application::OnFps const &) {}
    
    void BasicController::handle(events::application::OnResize const &) {}
    
    void BasicController::handle(events::application::OnMouseWheel const &) {}
    
    void BasicController::handle(events::application::OnMouseMove const &) {}
    
    void BasicController::handle(events::application::OnMouseDown const &) {}
    
    void BasicController::handle(events::application::OnMouseUp const &) {}
    
    void BasicController::handle(events::application::OnKeyUp const &) {}
    
    void BasicController::handle(events::application::OnKeyDown const &) {}

    void BasicController::handle(events::application::OnTextInput const &) {}
}