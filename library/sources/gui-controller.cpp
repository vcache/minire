#include <minire/gui-controller.hpp>

#include <minire/errors.hpp>
#include <minire/events/controller.hpp>
#include <minire/logging.hpp>

#include <gui-controller/builtin-theme.hpp>
#include <gui-controller/image-view.hpp>
#include <gui-controller/text-view.hpp>

#include <cassert>

namespace minire
{
    gui::ImageView::Sptr
    GuiController::makeImageView(content::Id const & textureId,
                                 utils::Patch const & patch)
    {
        auto result = std::make_shared<gui_controller::ImageViewImpl>(
            textureId, patch, *this);
        result->initialize();
        return result;
    }

    gui::TextView::Sptr
    GuiController::makeTextView(text::FormattedString const & text,
                                content::Id const & fontFace)
    {
        auto result = std::make_shared<gui_controller::TextViewImpl>(
            text, fontFace, *this);
        result->initialize();
        return result;
    }

    GuiController::~GuiController()
    {
        while(_overlays.size() > 1)
        {
            guiPop();
        }
    }

    gui::Component & GuiController::guiPush(
        std::string tag, models::InputHandler::Wptr fallbackHandler)
    {
        // cancel all in-progress operation in a current Overlay
        if (!_overlays.empty())
        {
            Overlay & overlay = topOverlay();

            // TODO: maybe focus shouldn't be lost, but instead
            //       frozen and returned after pop()
            if (auto focused = overlay._focused.lock(); focused)
            {
                focused->_hasFocus = false;
                focused->handle(gui::events::OnUnfocus{});
                overlay._focused.reset();
            }

            if (auto hovered = overlay._hovered.lock(); hovered)
            {
                hovered->_isHovered = false;
                hovered->handle(gui::events::OnMouseLeave{});
                overlay._hovered.reset();
            }

            if (auto clickTarget = overlay._toClick.lock(); clickTarget)
            {
                if (clickTarget->_isDragging)
                {
                    clickTarget->_isDragging = false;
                    clickTarget->handle(gui::events::OnDragEnd{std::nullopt});
                    _dragBegin = std::nullopt;
                }
                overlay._toClick.reset();
            }

            overlay._clickButton = {};
        }

        // create new overlay
        assert(_theme);
        auto root = std::make_shared<gui::Component>("__root__", *_theme, *this);
        root->_horizontal = gui::Arranger(gui::position::Constant{0}, gui::dimension::Fill{});
        root->_vertical = gui::Arranger(gui::position::Constant{0}, gui::dimension::Fill{});

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

        size_t offset = 1, i = 0;
        for(Overlay & overlay : _overlays)
        {
            assert(overlay._root);
            offset = std::max(offset, i * 10'000'000'000);
            offset = overlay._root->revalidate(offset, true, _windowArea, _windowArea);
            i++;
        }

        commitPendedViews();

        // force recalc of hovered item
        _mouseUpdated = true;
        hovered();
    }

    void GuiController::handle(events::application::OnResize const & e)
    {
        BasicController::handle(e);
        _windowArea = gui::Area{._left = 0, ._top = 0,
                                 ._width = static_cast<float>(e._width),
                                 ._height = static_cast<float>(e._height)};
    }

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
            focused->handle(e);
            return true;
        }
        else if (auto destination = hovered(); destination)
        {
            destination->handle(e);
            return true;
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
            clickTarget && clickTarget->_isDraggable.get() &&
            clickTarget->_isDragging)
        {
            assert(_dragBegin);
            clickTarget->handle(gui::events::OnDragMove{*_dragBegin, e});
        }

        if (auto destination = hovered(); destination)
        {
            destination->handle(e);
            return true;
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
            overlay._toClick = destination;
            overlay._clickButton = e._mouseButton;
            if (e._mouseButton == models::MouseButton::kLeft &&
                destination->_isDraggable.get())
            {
                assert(!destination->_isDragging);
                destination->_isDragging = true;
                destination->handle(gui::events::OnDragBegin{e});
                _dragBegin = e;
            }
            destination->handle(e);
            return true;
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
        if (clickTarget && clickTarget->_isDraggable.get() &&
            clickTarget->_isDragging)
        {
            clickTarget->_isDragging = false;
            clickTarget->handle(gui::events::OnDragEnd{e});
            _dragBegin = std::nullopt;
            overlay._toClick.reset();
        }

        if (auto destination = hovered(); destination)
        {
            if (clickTarget && clickTarget == destination &&
                e._mouseButton == overlay._clickButton)
            {
                overlay._toClick.reset();
                overlay._clickButton = {};

                setFocus(clickTarget->acceptFocus() ? clickTarget : nullptr);

                // NOTE: must not use "overlay" after handler(),
                //       because handle may close it
                clickTarget->handle(gui::events::OnClick{});
            }
            else
            {
                overlay._clickButton = {};
            }

            // TODO: should it be delivered to a non-click target?
            destination->handle(e);
            return true;
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
            focused->handle(e);
            return true;
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

        // Hotkeys are only applicable for the "__base__" overlay,
        // TODO: maybe hotkeys should be stored in an Overlay and
        //       be used on a per-Overlay manner.
        if (_overlays.size() == 1 && _hotKeys.handle(e))
        {
            return true;
        }

        Overlay & overlay = topOverlay();
        if (auto focused = overlay._focused.lock(); focused)
        {
            focused->handle(e);
            return true;
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
            focused->handle(e);
            return true;
        }
        else if (auto sink = overlay._fallbackHandler.lock(); sink)
        {
            return sink->handle(e);
        }

        return false;
    }

    bool GuiController::handle(events::application::OnClipboardUpdate const & e)
    {
        _clipboardState = e;

        if(BasicController::handle(e))
            return true;

        Overlay & overlay = topOverlay();
        if (auto focused = overlay._focused.lock(); focused)
        {
            focused->handle(e);
            return true;
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

        overlay._focused = component;

        if (focused)
        {
            focused->_hasFocus = false;
            focused->handle(gui::events::OnUnfocus{});
        }

        if (component)
        {
            component->_hasFocus = true;
            component->handle(gui::events::OnFocus{});
        }
    }

    void GuiController::setHover(gui::Component::Sptr const & component)
    {
        Overlay & overlay = topOverlay();
        auto hovered = overlay._hovered.lock();

        if (hovered == component)
            return;

        overlay._hovered = component;

        if (hovered)
        {
            hovered->_isHovered = false;
            hovered->handle(gui::events::OnMouseLeave{});
        }

        if (component)
        {
            component->_isHovered = true;
            component->handle(
                gui::events::OnMouseEnter{component == overlay._toClick.lock()});
        }
    }

    gui::Component::Sptr GuiController::hovered()
    {
        Overlay & overlay = topOverlay();

        if (auto result = overlay._hovered.lock();
            result && !_mouseUpdated)
        {
            if (result->_visible.get())
                return result;

            _mouseUpdated = true; // force recalc hovered
        }

        if (_mouseUpdated)
        {
            assert(overlay._root);
            _mouseUpdated = false;

            if (auto result = overlay._root->findUnderCursor(_mouseX, _mouseY))
            {
                if (result == overlay._root)
                    result.reset();
                setHover(result);
                return result;
            }
        }

        return {};
    }

    gui::Component const & GuiController::guiRoot() const
    {
        assert(!_overlays.empty());
        assert(_overlays.front()._root);
        return *(_overlays.front()._root);
    }

    gui::Component & GuiController::guiRoot()
    {
        assert(!_overlays.empty());
        assert(_overlays.front()._root);
        return *(_overlays.front()._root);
    }

    gui::Component const & GuiController::guiTop() const
    {
        Overlay const & overlay = topOverlay();
        assert(overlay._root);
        return *overlay._root;
    }

    gui::Component & GuiController::guiTop()
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

    gui::Component & GuiController::push(std::string const & tag,
                                          models::InputHandler::Wptr fallbackHandler)
    {
        return guiPush(tag,  fallbackHandler);
    }

    std::string const & GuiController::topTag() const
    {
        return guiTopTag();
    }

    void GuiController::pop()
    {
        guiPop();
    }

    void GuiController::startTextInput()
    {
        enqueue<events::controller::StartTextInput>();
    }

    void GuiController::stopTextInput()
    {
        enqueue<events::controller::StopTextInput>();
    }

    void GuiController::startClipboardCapture()
    {
        _clipboardState.reset();
        enqueue<events::controller::StartClipboardCapture>();
    }

    void GuiController::stopClipboardCapture()
    {
        enqueue<events::controller::StopClipboardCapture>();
        _clipboardState.reset();
    }

    void GuiController::setClipboardText(std::string const & text)
    {
        enqueue<events::controller::SetClipboardText>(text);
    }

    void GuiController::setPrimarySelection(std::string const & text)
    {
        enqueue<events::controller::SetPrimarySelection>(text);
    }

    std::string const * GuiController::getClipboardText() const
    {
        return _clipboardState ? &_clipboardState->_clipboardText : nullptr;
    }

    std::string const * GuiController::getPrimarySelection() const
    {
        return _clipboardState ? &_clipboardState->_primarySelection : nullptr;
    }

    void GuiController::setSystemCursor(::SDL_SystemCursor systemCursor)
    {
        enqueue<events::controller::SetSystemCursor>(systemCursor);
    }

    void GuiController::unfocus()
    {
        setFocus({});
    }

    gui::Area const & GuiController::topClientArea() const
    {
        return _windowArea;
    }

    void GuiController::set(::SDL_Scancode key, uint16_t mods,
                            gui::HotKeys::Handler handler)
    {
        _hotKeys.set(key, mods, std::move(handler));
    }

    void GuiController::enqueueView(gui::ContentView::Sptr const & contentView)
    {
        if (contentView)
        {
            _uncommittedViews.emplace_back(contentView);
        }
    }

    void GuiController::commitPendedViews()
    {
        for (gui::ContentView::Wptr const & wcontentView : _uncommittedViews)
        {
            if (auto const & contentView = wcontentView.lock();
                contentView)
            {
                contentView->commit();
            }
        }
        _uncommittedViews.clear();
    }

    std::unique_ptr<gui::Theme> GuiController::makeBuiltinTheme()
    {
        return gui_controller::makeDefaultTheme(contentManager(), *this);
    }
}
