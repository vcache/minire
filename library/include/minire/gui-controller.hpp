#pragma once

#include <minire/basic-controller.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/components/container.hpp>
#include <minire/gui/hot-keys.hpp>

#include <cassert>
#include <utility>

namespace minire
{
    class GuiController
        : public BasicController
    {
    public:
        template<typename ... Args>
        GuiController(Args &&... args)
            : BasicController(std::forward<Args>(args)...)
            , _guiRoot(std::make_shared<gui::components::Container>(*this, "", nullptr))
        {
            _guiRoot->setArrangers(gui::Arrangers
            {
                ._horizontal = gui::Arranger(gui::position::Constant{0}, gui::dimension::Fill{}),
                ._vertical = gui::Arranger(gui::position::Constant{0}, gui::dimension::Fill{}),
            });
        }

        void setFocus(gui::Component::Sptr const & = {});

    protected:
        void step() override;

        void handle(events::application::OnResize const &)  override;

        // NOTE: Descendant classes SHOULD call GuiController::handle
        //       before its own implementation.
        //       For details, see notes in basic-controller.hpp.

        void handle(events::application::OnFps const &) override;
        bool handle(events::application::OnMouseWheel const &) override;
        bool handle(events::application::OnMouseMove const &) override;
        bool handle(events::application::OnMouseDown const &) override;
        bool handle(events::application::OnMouseUp const &) override;
        bool handle(events::application::OnKeyUp const &) override;
        bool handle(events::application::OnKeyDown const &) override;
        bool handle(events::application::OnTextInput const &) override;
        void handle(events::application::OnRayCaster const &) override;

        gui::components::Container const & guiRoot() const
        {
            assert(_guiRoot);
            return *_guiRoot;
        }
        
        gui::components::Container & guiRoot()
        {
            assert(_guiRoot);
            return *_guiRoot;
        }

        void set(::SDL_Scancode key, uint16_t mods,
                 gui::HotKeys::Handler handler);

    private:
        void setHover(gui::Component::Sptr const &);

        gui::Component::Sptr hovered();

    private:
        // TODO: mouse-based events must be cancelled if some of
        //       SDL_WindowEvent have happened (alt+tab, minimization,
        //       unfocus, etc)

        gui::components::Container::Sptr _guiRoot;
        gui::Component::Wptr             _guiFocused;
        gui::Component::Wptr             _guiHovered;
        gui::Component::Wptr             _guiToClick;

        // TODO: hot keys must be cleared if some of SDL_WindowEvent
        //       have happened (alt+tab, minimization, unfocus, etc)
        // TODO: hot keys must fire an action not at KeyDown, but on
        //       a KeyUp and only if corresponding keys were pressed
        //       before (on an KeyDown event)
        gui::HotKeys                     _guiHotKeys;

        float                            _mouseX = -1;
        float                            _mouseY = -1;
        bool                             _mouseUpdated = true;
    };
}