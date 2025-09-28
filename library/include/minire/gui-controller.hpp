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

        void setFocus(gui::Component::Sptr = {});

    protected:
        void step() override;

        void handle(events::application::OnResize const &)  override;

        void handle(events::application::OnMouseWheel const &) override;
        void handle(events::application::OnMouseMove const &) override;
        void handle(events::application::OnMouseDown const &) override;
        void handle(events::application::OnMouseUp const &) override;
        void handle(events::application::OnKeyUp const &) override;
        void handle(events::application::OnKeyDown const &) override;
        void handle(events::application::OnTextInput const &) override;

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
        gui::components::Container::Sptr _guiRoot;
        gui::Component::Sptr             _guiFocused;
        gui::HotKeys                     _guiHotKeys;
    };
}