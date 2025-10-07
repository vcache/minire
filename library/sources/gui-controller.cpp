#include <minire/gui-controller.hpp>

#include <minire/errors.hpp>
#include <minire/events/controller.hpp>
#include <minire/logging.hpp>

#include <cassert>

namespace minire
{
    gui::components::Container & GuiController::guiPush(std::string tag,
                                                        models::InputHandler::Wptr fallbackHandler)
    {
        // cancel all in-progress operation in a current Overlay
        if (!_overlays.empty())
        {
            Overlay & overlay = topOverlay();
            overlay._focused.reset(); // TODO: send onUnfocus

            if (auto hovered = overlay._hovered.lock(); hovered)
            {
                hovered->_isHovered = false;
                hovered->onMouseLeave();
                overlay._hovered.reset();
            }

            if (auto clickTarget = overlay._toClick.lock(); clickTarget)
            {
                if (clickTarget->_isDragging)
                {
                    clickTarget->_isDragging = false;
                    clickTarget->onDragEnd(std::nullopt);
                }
                overlay._toClick.reset();
            }

            overlay._clickButton = {};
        }

        // create new overlay
        auto root = std::make_shared<gui::components::Container>(*this, "", nullptr);
        root->setArrangers(gui::Arrangers
        {
            ._horizontal = gui::Arranger(gui::position::Constant{0}, gui::dimension::Fill{}),
            ._vertical =   gui::Arranger(gui::position::Constant{0}, gui::dimension::Fill{}),
        });
        root->setClientArea(_windowArea);

        _overlays.push_back(Overlay
        {
            ._root = root,
            ._focused = {},
            ._hovered = {},
            ._toClick = {},
            ._clickButton = {},
            ._tag = std::move(tag),
            ._fallbackHandler = fallbackHandler,
        });

        _mouseUpdated = true; // force recalc hovered

        assert(root);
        return *root;
    }

    void GuiController::guiPop()
    {
        MINIRE_INVARIANT(_overlays.size() > 1,
                         "cannot pop the latest GUI overlay");
        _overlays.pop_back();
    }

    GuiController::Overlay & GuiController::topOverlay()
    {
        assert(!_overlays.empty());
        return _overlays.back();
    }

    GuiController::Overlay const & GuiController::topOverlay() const
    {
        assert(!_overlays.empty());
        return _overlays.back();
    }

    void GuiController::step()
    {
        BasicController::step();

        gui::Component::ZOrderUpdates labels;
        gui::Component::ZOrderUpdates sprites;

        size_t offset = 0, i = 0;
        for(Overlay & overlay : _overlays)
        {
            assert(overlay._root);
            offset = std::max(offset, i * 10'000'000'000);
            offset = overlay._root->revalidateZOrder(offset, labels, sprites);
            i++;
        }

        if (!labels.empty())
        {
            enqueue<events::controller::BulkSetLabelZOrders>(std::move(labels));
        }

        if (!sprites.empty())
        {
            enqueue<events::controller::BulkSetSpriteZOrders>(std::move(sprites));
        }

#       ifndef NDEBUG
        if (!labels.empty() || !sprites.empty())
        {
            MINIRE_DEBUG("zOrder change happened: {} labels and {} sprites",
                         labels.size(), sprites.size());
        }
#       endif
    }

    void GuiController::handle(events::application::OnResize const & e)
    {
        BasicController::handle(e);

        _windowArea = gui::Area{._left = 0, ._top = 0,
                                ._width = static_cast<float>(e._width),
                                ._height = static_cast<float>(e._height)};

        for(Overlay & overlay : _overlays)
        {
            assert(overlay._root);
            overlay._root->setClientArea(_windowArea);
        }
    }

    // TODO: following methods doesn't require to call Base::handle,
    //       maybe shouldn't do it, becase Derive may not call GuiController::handle at all
    //       (when it intercepts the input).

    void GuiController::handle(events::application::OnFps const & e)
    {
        BasicController::handle(e);
    }

    bool GuiController::handle(events::application::OnMouseWheel const & e)
    {
        if(BasicController::handle(e))
            return true;

        Overlay & overlay = topOverlay();

        if (auto focused = overlay._focused.lock(); focused)
        {
            return focused->handle(e);
        }
        else if (auto destination = hovered(); destination)
        {
            return destination->handle(e);
        }
        else if (auto sink = overlay._fallbackHandler.lock(); sink)
        {
            return sink->handle(e);
        }

        return false;
    }

    bool GuiController::handle(events::application::OnMouseMove const & e)
    {
        if(BasicController::handle(e))
            return true;

        _mouseX = e._absX;
        _mouseY = e._absY;
        _mouseUpdated = true;

        Overlay & overlay = topOverlay();

        if (auto clickTarget = overlay._toClick.lock();
            clickTarget && clickTarget->isDragable())
        {
            assert(clickTarget->_isDragging);
            clickTarget->onDragMove(e);
        }

        if (auto destination = hovered(); destination)
        {
            return destination->handle(e);
        }
        else if (auto sink = overlay._fallbackHandler.lock(); sink)
        {
            return sink->handle(e);
        }

        return false;
    }

    bool GuiController::handle(events::application::OnMouseDown const & e)
    {
        if(BasicController::handle(e))
            return true;

        Overlay & overlay = topOverlay();

        if (auto destination = hovered(); destination)
        {
            if (e._mouseButton == models::MouseButton::kLeft)
            {
                overlay._toClick = destination;
                overlay._clickButton = e._mouseButton;
                if (destination->isDragable())
                {
                    assert(!destination->_isDragging);
                    destination->_isDragging = true;
                    destination->onDragBegin(e);
                }
            }
            return destination->handle(e);
        }
        else if (auto sink = overlay._fallbackHandler.lock(); sink)
        {
            return sink->handle(e);
        }

        return false;
    }

    bool GuiController::handle(events::application::OnMouseUp const & e)
    {
        if(BasicController::handle(e))
            return true;

        Overlay & overlay = topOverlay();

        auto clickTarget = overlay._toClick.lock();
        if (clickTarget && clickTarget->isDragable())
        {
            assert(clickTarget->_isDragging);
            clickTarget->_isDragging = false;
            clickTarget->onDragEnd(e);
            overlay._toClick.reset();
        }

        if (auto destination = hovered(); destination)
        {
            if (clickTarget && clickTarget == destination &&
                e._mouseButton == overlay._clickButton)
            {
                clickTarget->onClick();
            }

            overlay._clickButton = {};

            return destination->handle(e); // TODO: should it be delivered to a non-click target?
        }
        else if (auto sink = overlay._fallbackHandler.lock(); sink)
        {
            return sink->handle(e);
        }

        return false;
    }

    bool GuiController::handle(events::application::OnKeyUp const & e)
    {
        if(BasicController::handle(e))
            return true;

        Overlay & overlay = topOverlay();
        if (auto focused = overlay._focused.lock(); focused)
        {
            return focused->handle(e);
        }
        else if (auto sink = overlay._fallbackHandler.lock(); sink)
        {
            return sink->handle(e);
        }

        return false;
    }

    bool GuiController::handle(events::application::OnKeyDown const & e)
    {
        if(BasicController::handle(e))
            return true;

        if (_hotKeys.handle(e))
        {
            return true;
        }

        Overlay & overlay = topOverlay();
        if (auto focused = overlay._focused.lock(); focused)
        {
            return focused->handle(e);
        }
        else if (auto sink = overlay._fallbackHandler.lock(); sink)
        {
            return sink->handle(e);
        }

        return false;
    }

    bool GuiController::handle(events::application::OnTextInput const & e)
    {
        if(BasicController::handle(e))
            return true;

        Overlay & overlay = topOverlay();
        if (auto focused = overlay._focused.lock(); focused)
        {
            return focused->handle(e);
        }
        else if (auto sink = overlay._fallbackHandler.lock(); sink)
        {
            return sink->handle(e);
        }

        return false;
    }

    void GuiController::handle(events::application::OnRayCaster const & e)
    {
        BasicController::handle(e);
    }

    void GuiController::setFocus(gui::Component::Sptr const & component)
    {
        Overlay & overlay = topOverlay();
        auto focused = overlay._focused.lock();

        if (focused == component)
            return;

        if (focused)
            focused->onUnfocus();

        overlay._focused = component;

        if (component)
            component->onFocus();
    }

    void GuiController::setHover(gui::Component::Sptr const & component)
    {
        Overlay & overlay = topOverlay();
        auto hovered = overlay._hovered.lock();

        if (hovered == component)
            return;

        if (hovered)
        {
            hovered->_isHovered = false;
            hovered->onMouseLeave();
        }

        overlay._hovered = component;

        if (component)
        {
            component->_isHovered = true;
            component->onMouseEnter(component == overlay._toClick.lock());
        }
    }

    // TODO: what if some Component suddenly appeared (become visible)
    //       under the mouse's cursor and then a MouseDown will happen w/o a MouseMove?
    gui::Component::Sptr GuiController::hovered()
    {
        Overlay & overlay = topOverlay();

        if (auto result = overlay._hovered.lock();
            result && !_mouseUpdated)
        {
            if (result->visible())
                return result;

            _mouseUpdated = true; // force recalc hovered
        }

        if (_mouseUpdated)
        {
            assert(overlay._root);
            if (auto result = overlay._root->findUnderCursor(_mouseX, _mouseY))
            {
                if (result == overlay._root)
                    result.reset();
                setHover(result);
                return result;
            }
            _mouseUpdated = false;
        }

        return {};
    }

    gui::components::Container const & GuiController::guiRoot() const
    {
        assert(!_overlays.empty());
        assert(_overlays.front()._root);
        return *(_overlays.front()._root);
    }

    gui::components::Container & GuiController::guiRoot()
    {
        assert(!_overlays.empty());
        assert(_overlays.front()._root);
        return *(_overlays.front()._root);
    }

    gui::components::Container const & GuiController::guiTop() const
    {
        Overlay const & overlay = topOverlay();
        assert(overlay._root);
        return *overlay._root;
    }

    gui::components::Container & GuiController::guiTop()
    {
        Overlay & overlay = topOverlay();
        assert(overlay._root);
        return *overlay._root;
    }

    std::string const & GuiController::guiTopTag() const
    {
        Overlay const & overlay = topOverlay();
        assert(overlay._root);
        return overlay._tag;
    }

    void GuiController::set(::SDL_Scancode key, uint16_t mods,
                            gui::HotKeys::Handler handler)
    {
        _hotKeys.set(key, mods, std::move(handler));
    }
}