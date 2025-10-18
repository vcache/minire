#pragma once

#include <minire/gui/area.hpp>
#include <minire/models/input-handler.hpp>

#include <memory>
#include <string>

namespace minire::gui
{
    class Component;

    class OverlayController
    {
    public:
        virtual ~OverlayController() = default;

        virtual Component & push(std::string const & tag,
                                 minire::models::InputHandler::Wptr = {}) = 0;

        virtual std::string const & topTag() const = 0;

        virtual Area const & topClientArea() const = 0;

        virtual void pop() = 0;
    };
}