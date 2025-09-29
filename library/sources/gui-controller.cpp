#include <minire/gui-controller.hpp>

#include <minire/events/controller.hpp>

#include <cassert>

namespace minire
{
    void GuiController::step()
    {
        BasicController::step();

        assert(_guiRoot);

        gui::Component::ZOrderUpdates labels;
        gui::Component::ZOrderUpdates sprites;

        _guiRoot->revalidateZOrder(0, labels, sprites);

        if (!labels.empty())
        {
            enqueue<events::controller::BulkSetLabelZOrders>(std::move(labels));
        }

        if (!sprites.empty())
        {
            enqueue<events::controller::BulkSetSpriteZOrders>(std::move(sprites));
        }
    }

    void GuiController::handle(events::application::OnResize const & e)
    {
        BasicController::handle(e);
        guiRoot().setClientArea(gui::Area{._left = 0, ._top = 0,
                                          ._width = static_cast<float>(e._width),
                                          ._height = static_cast<float>(e._height)});
    }

    // TODO: following methods doesn't require to call Base::handle,
    //       maybe shouldn't do it, becase Derive may not call GuiController::handle at all
    //       (when it intercepts the input).

    void GuiController::handle(events::application::OnFps const & e)
    {
        BasicController::handle(e);
    }

    void GuiController::handle(events::application::OnMouseWheel const & e)
    {
        BasicController::handle(e);
        if (_guiFocused)
        {
            _guiFocused->handle(e);
        }
    }

    void GuiController::handle(events::application::OnMouseMove const & e)
    {
        BasicController::handle(e);
        assert(_guiRoot);
        _guiRoot->handle(e);
    }

    void GuiController::handle(events::application::OnMouseDown const & e)
    {
        BasicController::handle(e);
        assert(_guiRoot);
        _guiRoot->handle(e);
    }

    void GuiController::handle(events::application::OnMouseUp const & e)
    {
        BasicController::handle(e);
        assert(_guiRoot);
        _guiRoot->handle(e);
    }

    void GuiController::handle(events::application::OnKeyUp const & e)
    {
        BasicController::handle(e);
        if (_guiFocused)
        {
            _guiFocused->handle(e);
        }
    }

    void GuiController::handle(events::application::OnKeyDown const & e)
    {
        BasicController::handle(e);
        if (!_guiHotKeys.handle(e) && _guiFocused)
        {
            _guiFocused->handle(e);
        }
    }

    void GuiController::handle(events::application::OnTextInput const & e)
    {
        BasicController::handle(e);
        if (_guiFocused)
        {
            _guiFocused->handle(e);
        }
    }

    void GuiController::handle(events::application::OnRayCaster const & e)
    {
        BasicController::handle(e);
    }

    void GuiController::setFocus(gui::Component::Sptr component)
    {
        _guiFocused = component;
    }

    void GuiController::set(::SDL_Scancode key, uint16_t mods,
                            gui::HotKeys::Handler handler)
    {
        _guiHotKeys.set(key, mods, std::move(handler));
    }
}