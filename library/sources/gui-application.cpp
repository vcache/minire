#include <minire/gui-application.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/gui/events.hpp>
#include <minire/logging.hpp>

#include <gui-application/builtin-theme.hpp>
#include <minire/utils/rect.hpp>
#include <text/measurer.hpp>
#include <utils/overloaded.hpp>

#include <cassert>

namespace minire
{
    GuiApplication::~GuiApplication()
    {
        while(_overlays.size() > 1) // don't pop the first one
        {
            guiPop();
        }
    }

    gui::Component & GuiApplication::guiPush(
        std::string tag, application::InputHandler::Wptr fallbackHandler)
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
                focused->handle(gui::OnUnfocus{});
                overlay._focused.reset();
            }

            if (auto hovered = overlay._hovered.lock(); hovered)
            {
                hovered->_isHovered = false;
                hovered->handle(gui::OnMouseLeave{});
                overlay._hovered.reset();
            }

            if (auto clickTarget = overlay._toClick.lock(); clickTarget)
            {
                if (clickTarget->_isDragging)
                {
                    clickTarget->_isDragging = false;
                    clickTarget->handle(gui::OnDragEnd{std::nullopt});
                    _dragBegin = std::nullopt;
                }
                overlay._toClick.reset();
            }

            overlay._clickButton = {};
        }

        // create new overlay
        assert(_theme);
        auto root = std::make_shared<gui::Component>("__root__", *_theme, gui::Theme::Style{}, *this);
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

    void GuiApplication::guiPop()
    {
        MINIRE_INVARIANT(_overlays.size() > 1,
                         "cannot pop the latest GUI overlay");
        _overlays.pop_back();
    }

    GuiApplication::Overlay & GuiApplication::topOverlay()
    {
        assert(!_overlays.empty());
        return _overlays.back();
    }

    GuiApplication::Overlay const & GuiApplication::topOverlay() const
    {
        assert(!_overlays.empty());
        return _overlays.back();
    }

    void GuiApplication::onStart()
    {
        Application::onStart();
        GuiApplication::onStep();
    }

    bool GuiApplication::onStep()
    {
        bool result = Application::onStep();

        bool invalidated = true;
        size_t iterations = 0;
        while(invalidated && iterations < 5)
        {
            invalidated = false;

            size_t offset = 1, i = 0;
            for(Overlay & overlay : _overlays)
            {
                assert(overlay._root);
                offset = std::max(offset, i * 10'000'000'000);
                offset = overlay._root->revalidate(offset, true, _windowArea, _windowArea);
                i++;
                invalidated |= overlay._root->invalidated();
            }

            iterations++;
        }

        if (iterations > 2)
        {
            MINIRE_WARNING("gui loop probably detected: {} iterations", iterations);
        }

        // force recalc of hovered item
        _mouseUpdated = true;
        hovered();

        auto hoveredComp = hovered();
        setSystemCursor(hoveredComp ? hoveredComp->systemCursor()
                                    : models::SystemCursor::kArrow);

        return result; // not add "true" since Gui don't need lerping
    }

    bool GuiApplication::handle(application::OnMouseWheel const & e)
    {
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

        return Application::handle(e);
    }

    bool GuiApplication::handle(application::OnMouseMove const & e)
    {
        _mouseX = e._absX;
        _mouseY = e._absY;
        _mouseUpdated = true;

        Overlay & overlay = topOverlay();

        if (auto clickTarget = overlay._toClick.lock();
            clickTarget && clickTarget->_isDraggable.get() &&
            clickTarget->_isDragging)
        {
            assert(_dragBegin);
            clickTarget->handle(gui::OnDragMove{*_dragBegin, e});
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

        return Application::handle(e);
    }

    bool GuiApplication::handle(application::OnMouseDown const & e)
    {
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
                destination->handle(gui::OnDragBegin{e});
                _dragBegin = e;
            }
            destination->handle(e);
            return true;
        }
        else if (auto sink = overlay._fallbackHandler.lock(); sink)
        {
            return sink->handle(e);
        }

        return Application::handle(e);
    }

    bool GuiApplication::handle(application::OnMouseUp const & e)
    {
        Overlay & overlay = topOverlay();
        auto clickTarget = overlay._toClick.lock();

        if (clickTarget && clickTarget->_isDraggable.get() &&
            clickTarget->_isDragging)
        {
            clickTarget->_isDragging = false;
            clickTarget->handle(gui::OnDragEnd{e});
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
                clickTarget->handle(gui::OnClick{});
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

        return Application::handle(e);
    }

    bool GuiApplication::handle(application::OnKeyUp const & e)
    {
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

        return Application::handle(e);
    }

    bool GuiApplication::handle(application::OnKeyDown const & e)
    {
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

        return Application::handle(e);
    }

    bool GuiApplication::handle(application::OnTextInput const & e)
    {
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

        return Application::handle(e);
    }

    void GuiApplication::setFocus(gui::Component::Sptr const & component)
    {
        Overlay & overlay = topOverlay();
        auto focused = overlay._focused.lock();

        if (focused == component)
            return;

        overlay._focused = component;

        if (focused)
        {
            focused->_hasFocus = false;
            focused->handle(gui::OnUnfocus{});
        }

        if (component)
        {
            component->_hasFocus = true;
            component->handle(gui::OnFocus{});
        }
    }

    void GuiApplication::setHover(gui::Component::Sptr const & component)
    {
        Overlay & overlay = topOverlay();
        auto hovered = overlay._hovered.lock();

        if (hovered == component)
            return;

        overlay._hovered = component;

        if (hovered)
        {
            hovered->_isHovered = false;
            hovered->handle(gui::OnMouseLeave{});
        }

        if (component)
        {
            component->_isHovered = true;
            component->handle(
                gui::OnMouseEnter{component == overlay._toClick.lock()});
        }
    }

    gui::Component::Sptr GuiApplication::hovered()
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

    gui::Component const & GuiApplication::guiRoot() const
    {
        assert(!_overlays.empty());
        assert(_overlays.front()._root);
        return *(_overlays.front()._root);
    }

    gui::Component & GuiApplication::guiRoot()
    {
        assert(!_overlays.empty());
        assert(_overlays.front()._root);
        return *(_overlays.front()._root);
    }

    gui::Component const & GuiApplication::guiTop() const
    {
        Overlay const & overlay = topOverlay();
        assert(overlay._root);
        return *overlay._root;
    }

    gui::Component & GuiApplication::guiTop()
    {
        Overlay & overlay = topOverlay();
        assert(overlay._root);
        return *overlay._root;
    }

    std::string const & GuiApplication::guiTopTag() const
    {
        Overlay const & overlay = topOverlay();
        assert(overlay._root);
        return overlay._tag;
    }

    gui::Component & GuiApplication::push(std::string const & tag,
                                          application::InputHandler::Wptr fallbackHandler)
    {
        return guiPush(tag,  fallbackHandler);
    }

    std::string const & GuiApplication::topTag() const
    {
        return guiTopTag();
    }

    void GuiApplication::pop()
    {
        guiPop();
    }

    void GuiApplication::startTextInput()
    {
        Application::startTextInput();
    }

    void GuiApplication::stopTextInput()
    {
        Application::stopTextInput();
    }

    void GuiApplication::setClipboardText(std::string const & text)
    {
        Application::setClipboardText(text);
    }

    void GuiApplication::setPrimarySelection(std::string const & text)
    {
        Application::setPrimarySelection(text);
    }

    std::string GuiApplication::clipboardText() const
    {
        return Application::clipboardText();
    }

    std::string GuiApplication::primarySelection() const
    {
        return Application::primarySelection();
    }

    void GuiApplication::setSystemCursor(models::SystemCursor const systemCursor)
    {
        if (_systemCursor != systemCursor)
        {
            _systemCursor = systemCursor;
            Application::setSystemCursor(_systemCursor);
        }
    }

    void GuiApplication::unfocus()
    {
        setFocus({});
    }

    gui::Area const & GuiApplication::topClientArea() const
    {
        return _windowArea;
    }

    void GuiApplication::set(::SDL_Scancode key, uint16_t mods,
                             gui::HotKeys::Handler handler)
    {
        _hotKeys.set(key, mods, std::move(handler));
    }

    Sprite::Sptr GuiApplication::make(minire::models::Sprite model)
    {
        return Application::make(std::move(model));
    }

    Label::Sptr GuiApplication::make(minire::models::Label model)
    {
        return Application::make(std::move(model));
    }

    std::pair<glm::vec2, bool>
    GuiApplication::measure(minire::models::sprite::Image const & image) const
    {
        return Application::measure(image);
    }

    glm::vec2 GuiApplication::measure(text::FormattedString const & text,
                                      content::Id const & fontFace) const
    {
        return Application::measure(text, fontFace);
    }

    std::unique_ptr<utils::TextLayout> GuiApplication::layout(text::FormattedString const & text,
                                                              content::Id const & fontFace) const
    {
        return Application::layout(text, fontFace);
    }

    std::unique_ptr<gui::Theme> GuiApplication::makeBuiltinTheme()
    {
        return gui_application::makeDefaultTheme(contentManager());
    }

    bool GuiApplication::handle(application::OnResize const & e)
    {
        _windowArea = gui::Area
        {
            ._left = 0,
            ._top = 0,
            ._width = static_cast<float>(e._width),
            ._height = static_cast<float>(e._height),
        };

        return Application::handle(e);
    }
}
