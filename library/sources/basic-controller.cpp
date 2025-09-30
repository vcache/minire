#include <minire/basic-controller.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/unow.hpp>

#include <utils/fps-counter.hpp>
#include <text/measurer.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <variant>

namespace minire
{
    BasicController::BasicController(content::Manager & contentMangager,
                                     size_t const maxFps)
        : _contentManager(contentMangager)
        , _maxFps(maxFps)
        , _working(false)
        , _quitRequest(false)
        , _lowLatencyInput(false)
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
    
    void BasicController::run(events::application::OnResize const & initial)
    {
        _thread = std::thread(
            [this, &initial]
            {
                try
                {
                    worker(initial);
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

    void BasicController::worker(events::application::OnResize const & initial)
    {
        MINIRE_INVARIANT(initial._width > 0 && initial._height > 0,
                         "bad initial screen size: {}x{}", initial._width, initial._height);

        start();
        handle(initial);
        finishCurrentBatch(0.0);
        _initBarrier.notify();

        size_t frameBegin = utils::uNow(), frameEnd; // microseconds
        size_t const frameQuant = size_t(1e6 / static_cast<double>(_maxFps));
        _frameTime = static_cast<double>(frameQuant) / 1e6;

        utils::FpsCounter fpsCounter(2); (void) fpsCounter;
        _working = true;
        while(_working)
        {
            // handle input events
            events::ApplicationQueue pendedEvents;
            {
                std::lock_guard<std::mutex> lock(_applicationEventsMutex);
                std::swap(pendedEvents, _applicationEvents);
                _applicationEvents.reserve(pendedEvents.size());
            }
            handle(pendedEvents);

            // do a logic step
            step();

            // do a post-processing step
            postprocess();

            if (_quitRequest)
            {
                _working = false;
                enqueue<events::controller::Quit>();
                finishCurrentBatch(_frameTime);
                continue;
            }

            // sleep until frame's quant is done or an urgent event arrived
            size_t const timeSpent = utils::uNow() - frameBegin;
            if (timeSpent < frameQuant)
            {
                size_t const timeLeft = frameQuant - timeSpent;
                std::unique_lock<std::mutex> lock(_applicationEventsMutex);
                _applicationEventsCond.wait_for(lock, std::chrono::microseconds(timeLeft),
                                                [this] { return !_applicationEvents.empty(); });
            }

            // collect frame statistics
            frameEnd = utils::uNow();
            assert(frameBegin <= frameEnd);
            _frameTime = double(frameEnd - frameBegin) / 1e6;
            _frameTime = std::min(1.0, _frameTime); // prevent from going haywire
            _absoluteTime += _frameTime;

            // signal render thread about logic frame finish
            // TODO: maybe do this before the sleep? Because otherwise
            //       it won't start smoothly with very slow controller (like 1 FPS).
            //       Also, it might decrease latency between a controller and a rasterizer.
            finishCurrentBatch(_frameTime);

            // prepare next frame
            frameBegin = frameEnd;

#ifndef NDEBUG
            // count FPS of a controller
            if (auto fps = fpsCounter.registerFrame(); fps)
            {
                std::string title = fmt::format("[{}  fps, mft = {:.4f} ms]",
                                                fps->first, fps->second);
                MINIRE_DEBUG("Controller FPS: {}", title);

                if (_eventsLatency.size() >= 2)
                {
                    std::ranges::sort(_eventsLatency);
                    MINIRE_DEBUG("Events latency: p0 = {}; p50 = {}; p90 = {}, p99 = {}, p100 = {}",
                                 _eventsLatency.front(),
                                 _eventsLatency[std::lround(static_cast<float>(_eventsLatency.size()) * .50f)],
                                 _eventsLatency[std::lround(static_cast<float>(_eventsLatency.size()) * .90f)],
                                 _eventsLatency[std::lround(static_cast<float>(_eventsLatency.size()) * .99f)],
                                 _eventsLatency.back());
                    _eventsLatency.clear();
                }
            }
#endif
        }

        finish();

        finishCurrentBatch(_frameTime);
    }

    BasicController::BatchQueue BasicController::pull()
    {
        BatchQueue result;
        // TODO this is unsafe: result.reserve(_pendedControllerEvents.size());
        {
            std::lock_guard<std::mutex> lock(_pendedControllerEventsMutex);
            std::swap(result, _pendedControllerEvents);
        }
        return result; // TODO: is it reallty moves and don't copy?
    }

    void BasicController::finishCurrentBatch(double duration)
    {
        _currentEventsBatch._duration = duration;
        {
            std::lock_guard<std::mutex> lock(_pendedControllerEventsMutex);
            _pendedControllerEvents.emplace_back(std::move(_currentEventsBatch));
        }
        _currentEventsBatch = Batch();
        // TODO: reserve max() or p99 of known events amount _currentEventsBatch._events.reserve()
    }

    void BasicController::push(events::ApplicationQueue && applicationQueue)
    {
        if (applicationQueue.empty())
            return;

        {
            std::lock_guard<std::mutex> lock(_applicationEventsMutex);
            _applicationEvents.reserve(_applicationEvents.size() + applicationQueue.size());
            std::move(applicationQueue.begin(), applicationQueue.end(),
                      std::back_inserter(_applicationEvents));
        }

        if (_lowLatencyInput)
        {
            _applicationEventsCond.notify_one();
        }
    }

    void BasicController::quit()
    {
        _quitRequest = true;
    }

    std::unique_ptr<content::Lease>
    BasicController::borrow(content::Id const & id) const
    {
        return _contentManager.borrow(id);
    }

    glm::vec2 BasicController::measure(text::FormattedString const & text,
                                       content::Id const & id) const
    {
        auto lease = _contentManager.borrow(id);
        assert(lease);
        models::FontFace const & fontFace = lease->as<models::FontFace>();
        return measure(text, fontFace);
    }

    glm::vec2 BasicController::measure(text::FormattedString const & text,
                                       models::FontFace const & fontFace) const
    {
        return text::measure(text, fontFace);

    }

    void BasicController::setLowLatencyInput(bool enabled)
    {
        _lowLatencyInput = enabled;
    }

    bool BasicController::lowLatencyInput() const
    {
        return _lowLatencyInput;
    }

    void BasicController::enqueueRaw(events::Controller && event)
    {
        _currentEventsBatch._events.emplace_back(std::move(event));
    }

    void BasicController::handle(events::ApplicationQueue const & events)
    {
#       ifndef NDEBUG
        size_t const now = utils::uNow();
#       else
        size_t constexpr now = 0;
#       endif

        for(auto const & event: events)
        {
            std::visit([this, now](auto const & e)
                {
#                   ifndef NDEBUG
                    _eventsLatency.push_back(now - e._createTime);
#                   endif

                    handle(e);
                }, event);
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

    void BasicController::handle(events::application::OnRayCaster const &) {}
}
