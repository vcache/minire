#pragma once

#include <minire/application/events.hpp>

#include <optional>

// GUI-specific events

namespace minire::gui
{
    struct OnDragBegin
    {
        application::OnMouseDown _event;
    };

    struct OnDragMove
    {
        application::OnMouseDown _begin;
        application::OnMouseMove _event;
    };

    struct OnDragEnd
    {
        std::optional<application::OnMouseUp> _event;
    };

    struct OnMouseEnter
    {
        bool _isClickReturn;    // TODO: maybe use Drag logics instead?
    };

    struct OnMouseLeave {};

    struct OnFocus {};

    struct OnUnfocus {};

    struct OnClick {};
}
