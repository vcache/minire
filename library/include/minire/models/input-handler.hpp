#pragma once

#include <minire/events/application/on-key-down.hpp>
#include <minire/events/application/on-key-up.hpp>
#include <minire/events/application/on-mouse-down.hpp>
#include <minire/events/application/on-mouse-move.hpp>
#include <minire/events/application/on-mouse-up.hpp>
#include <minire/events/application/on-mouse-wheel.hpp>
#include <minire/events/application/on-text-input.hpp>

#include <memory>

namespace minire::models
{
    class InputHandler
    {
    public:
        using Sptr = std::shared_ptr<InputHandler>;
        using Wptr = std::weak_ptr<InputHandler>;

        explicit InputHandler(bool defaultResult = false)
            : _defaultResult(defaultResult)
        {}

        virtual ~InputHandler() = default;

        // NOTE: input events are returning boolean value that
        //       indicates a consumation of an event.
        //       Normally, consumed events shouldn't be processed
        //       by a descendant classes. The following pattern is
        //       recommended (usually make sense w/ GuiController):
        //
        //       class Foo : public GuiController
        //       {
        //           bool handle(OnMouseDown const & e) override
        //           {
        //               if (GuiController::handle(e))
        //                   return true;
        //               ...
        //               return true;
        //           }
        //       };
        //
        //       It is also legal to not call Base class's handle() in
        //       situations when descedant's intent is to capture inputs
        //       exclusively (for example, when moving a camera by a
        //       mouse and need to prevent activation of GUI elements).

        virtual bool handle(events::application::OnMouseWheel const &)      { return _defaultResult; }
        virtual bool handle(events::application::OnMouseMove const &)       { return _defaultResult; }
        virtual bool handle(events::application::OnMouseDown const &)       { return _defaultResult; }
        virtual bool handle(events::application::OnMouseUp const &)         { return _defaultResult; }
        virtual bool handle(events::application::OnKeyUp const &)           { return _defaultResult; }
        virtual bool handle(events::application::OnKeyDown const &)         { return _defaultResult; }
        virtual bool handle(events::application::OnTextInput const &)       { return _defaultResult; }
        virtual bool handle(events::application::OnClipboardUpdate const &) { return _defaultResult; }

    private:
        bool _defaultResult;
    };
}
