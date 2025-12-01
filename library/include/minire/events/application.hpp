#pragma once

#include <minire/events/application/on-fps.hpp>
#include <minire/events/application/on-resize.hpp>
#include <minire/events/application/on-mouse-wheel.hpp>
#include <minire/events/application/on-mouse-move.hpp>
#include <minire/events/application/on-mouse-down.hpp>
#include <minire/events/application/on-mouse-up.hpp>
#include <minire/events/application/on-key-up.hpp>
#include <minire/events/application/on-key-down.hpp>
#include <minire/events/application/on-text-input.hpp>
#include <minire/events/application/on-ray-caster.hpp>

#include <variant>
#include <vector>

namespace minire::events
{
    using Application = std::variant<application::OnFps,
                                     application::OnResize,
                                     application::OnMouseWheel,
                                     application::OnMouseMove,
                                     application::OnMouseDown,
                                     application::OnMouseUp,
                                     application::OnKeyUp,
                                     application::OnKeyDown,
                                     application::OnTextInput,
                                     application::OnRayCaster>;

    // TODO: move this to minire/events/application.hpp
    // TODO: maybe deque?
    using ApplicationQueue = std::vector<Application>;
}
