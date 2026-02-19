#pragma once

#include <minire/gui/area.hpp>
#include <minire/models/input-handler.hpp>

#include <SDL2/SDL_mouse.h>

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

    public:
        // TODO: are they supposed to live here? They aren't directly
        //       connected to overlays.

        virtual void startTextInput() = 0;

        virtual void stopTextInput() = 0;

        virtual void startClipboardCapture() = 0;

        virtual void stopClipboardCapture() = 0;

        virtual void setClipboardText(std::string const &) = 0;

        virtual void setPrimarySelection(std::string const &) = 0;

        virtual std::string const * getClipboardText() const = 0;

        virtual std::string const * getPrimarySelection() const = 0;

        virtual void setSystemCursor(::SDL_SystemCursor) = 0;

        virtual void unfocus() = 0;
    };
}
