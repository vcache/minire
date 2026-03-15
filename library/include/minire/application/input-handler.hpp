#pragma once

#include <minire/application/events.hpp>

#include <memory>

namespace minire::application
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
        //       recommended (usually make sense w/ GuiApplication):
        //
        //       class Foo : public ::minire::GuiApplication
        //       {
        //           bool handle(OnMouseDown const & e) override
        //           {
        //               if (GuiApplication::handle(e))
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

        virtual bool handle(application::OnResize const &)     { return _defaultResult; }
        virtual bool handle(application::OnMouseWheel const &) { return _defaultResult; }
        virtual bool handle(application::OnMouseMove const &)  { return _defaultResult; }
        virtual bool handle(application::OnMouseDown const &)  { return _defaultResult; }
        virtual bool handle(application::OnMouseUp const &)    { return _defaultResult; }
        virtual bool handle(application::OnKeyUp const &)      { return _defaultResult; }
        virtual bool handle(application::OnKeyDown const &)    { return _defaultResult; }
        virtual bool handle(application::OnTextInput const &)  { return _defaultResult; }

    private:
        bool _defaultResult;
    };
}
