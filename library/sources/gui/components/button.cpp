#include <minire/gui/components/button.hpp>

#include <minire/errors.hpp>

namespace minire::gui::components
{
    Button::Button(std::string const & id,
                   Theme const & theme,
                   Theme::Style const & style,
                   OverlayController & overlayController)
        : Component(id, theme, style, overlayController)
        , _bgNormal(*this, theme.makeImage("button", "bg-normal", style))
        , _bgHovered(*this, theme.makeImage("button", "bg-hovered", style))
        , _bgPressed(*this, theme.makeImage("button", "bg-pressed", style))
        , _text(*this)
        , _icon(*this)
        , _iconLocation(*this, theme.parameter<Theme::Location>("button", "icon-location", style))
        , _iconSpacing(*this, theme.parameter<float>("button", "icon-spacing", style))
        , _pressOffset(*this, theme.parameter<glm::vec2>("button", "press-offset", style))
    {
        padding() = theme.parameter<utils::Rect>("button", "padding", style);
    }

    template<typename T>
    void Button::actualize(Property<std::shared_ptr<T>> & contentView)
    {
        if (contentView.isInvalidated() && contentView.get())
        {
            contentView.get()->setContentInvalidator(shared_from_this());
        }
    }

    size_t Button::revalidateContent(size_t zOffset,
                                     bool const effectiveVisible,
                                     Area const & contentArea,
                                     Area const & clippingWindow)
    {
        // update content invalidator
        actualize(_bgNormal);
        actualize(_bgHovered);
        actualize(_bgPressed);
        actualize(_text);
        actualize(_icon);

        // revalidate visibility
        ImageView::Sptr const & background = activeBackground();
        if (auto const & p = _bgNormal.get()) p->setVisible((_bgNormal.get() == background) && effectiveVisible);
        if (auto const & p = _bgHovered.get()) p->setVisible((_bgHovered.get() == background) && effectiveVisible);
        if (auto const & p = _bgPressed.get()) p->setVisible((_bgPressed.get() == background) && effectiveVisible);

        if (auto const & icon = _icon.get()) icon->setVisible(effectiveVisible);
        if (auto const & text = _text.get()) text->setVisible(effectiveVisible);

        // revalidate positions
        revalidatePositions(contentArea, clippingWindow);

        // revalidate zOrder
        if (auto const & p = _bgNormal.get()) zOffset = p->onZOrderChanged(zOffset);
        if (auto const & p = _bgHovered.get()) zOffset = p->onZOrderChanged(zOffset);
        if (auto const & p = _bgPressed.get()) zOffset = p->onZOrderChanged(zOffset);
        if (auto const & p = _text.get()) zOffset = p->onZOrderChanged(zOffset);
        if (auto const & p = _icon.get()) zOffset = p->onZOrderChanged(zOffset);

        // finish
        _bgNormal.revalidate();
        _bgHovered.revalidate();
        _bgPressed.revalidate();
        _text.revalidate();
        _icon.revalidate();
        _iconLocation.revalidate();
        _iconSpacing.revalidate();
        _pressOffset.revalidate();

        return zOffset;
    }

    std::optional<std::pair<float, float>> Button::measureContent() const
    {
        float const extraWidth = padding().get()._left + padding().get()._right;
        float const extraHeight = padding().get()._top + padding().get()._bottom;
        bool const hasIcon = _icon.get().operator bool();
        bool const hasText = _text.get().operator bool();

        if (!hasIcon && !hasText)
        {
            return std::make_pair(extraWidth, extraHeight);
        }
        else if (!hasIcon)
        {
            // only a text
            assert(hasText && _text.get());
            auto [size, _] = _text.get()->measure();
            return std::make_pair(size.x + extraWidth, size.y + extraHeight);
        }
        else if (!hasText)
        {
            // only an icon
            assert(hasIcon && _icon.get());
            auto [size, _] = _icon.get()->measure();
            return std::make_pair(size.x + extraWidth, size.y + extraHeight);
        }
        else
        {
            // an icon and a text
            assert(hasIcon && _icon.get());
            assert(hasText && _text.get());
            auto [textSize, _] = _text.get()->measure();
            auto [iconSize, __] = _icon.get()->measure();
            switch(_iconLocation.get())
            {
                case Theme::Location::kLeft:
                case Theme::Location::kRight:
                    return std::make_pair(iconSize.x + textSize.x + _iconSpacing.get() + extraWidth,
                                          std::max(iconSize.y, textSize.y) + extraHeight);

                case Theme::Location::kTop:
                case Theme::Location::kBottom:
                    return std::make_pair(std::max(iconSize.x, textSize.x) + extraWidth,
                                          iconSize.y + textSize.y + _iconSpacing.get() + extraHeight);
            }

            MINIRE_THROW("unknown icon position: {}", static_cast<int>(_iconLocation.get()));
        }
    }

    ImageView::Sptr const & Button::activeBackground() const
    {
        switch(_state)
        {
            case State::kNormal:  return _bgNormal.get();
            case State::kHovered: return _bgHovered.get();
            case State::kPressed: return _bgPressed.get();
        }
        MINIRE_THROW("unknown button state: {}", static_cast<int>(_state));
    }

    void Button::revalidatePositions(Area const & contentArea,
                                     Area const & clippingWindow)
    {
        for (auto const & bg : {_bgNormal.get(), _bgHovered.get(), _bgPressed.get()})
        {
            if (!bg) continue;
            bg->setContentArea(contentArea);
            bg->setClippingWindow(clippingWindow);
        }

        auto const & icon = _icon.get();
        auto const & text = _text.get();
        bool const hasIcon = icon.operator bool();
        bool const hasText = text.operator bool();

        glm::vec2 const offset = _state == State::kPressed ? _pressOffset.get()
                                                           : glm::vec2();

        if (!hasIcon && !hasText)
        {
            // nothing to do
        }
        else if (!hasIcon)
        {
            // only a text
            assert(hasText && text);
            auto [size, _] = text->measure();
            _textPosition.x = contentArea._left + (contentArea._width - size.x) / 2.0f;
            _textPosition.y = contentArea._top + (contentArea._height - size.y) / 2.0f;
            text->setContentPosition(_textPosition.x + offset.x, _textPosition.y + offset.y);
            text->setClippingWindow(clippingWindow);
        }
        else if (!hasText)
        {
            // only an icon
            assert(hasIcon && icon);
            auto [size, _] = icon->measure();
            _iconPosition.x = contentArea._left + (contentArea._width - size.x) / 2.0f;
            _iconPosition.y = contentArea._top + (contentArea._height - size.y) / 2.0f;
            icon->setContentPosition(_iconPosition.x + offset.x, _iconPosition.y + offset.y);
            icon->setClippingWindow(clippingWindow);
        }
        else
        {
            // an icon and a text
            assert(hasIcon && icon);
            assert(hasText && text);
            auto [textSize, _] = text->measure();
            auto [iconSize, __] = icon->measure();

            float const totalWidth = iconSize.x + textSize.x + _iconSpacing.get();
            float const totalHeight = iconSize.y + textSize.y + _iconSpacing.get();

            switch(_iconLocation.get())
            {
                case Theme::Location::kLeft:
                    _iconPosition.x = contentArea._left + (contentArea._width - totalWidth) / 2.0f;
                    _textPosition.x = _iconPosition.x + iconSize.x + _iconSpacing.get();
                    _iconPosition.y = contentArea._top + (contentArea._height - iconSize.y) / 2.0f;
                    _textPosition.y = contentArea._top + (contentArea._height - textSize.y) / 2.0f;
                    break;

                case Theme::Location::kTop:
                    _iconPosition.x = contentArea._left + (contentArea._width - iconSize.x) / 2.0f;
                    _textPosition.x = contentArea._left + (contentArea._width - textSize.x) / 2.0f;
                    _iconPosition.y = contentArea._top + (contentArea._height - totalHeight) / 2.0f;
                    _textPosition.y = _iconPosition.y + iconSize.y + _iconSpacing.get();
                    break;

                case Theme::Location::kRight:
                    _textPosition.x = contentArea._left + (contentArea._width - totalWidth) / 2.0f;
                    _iconPosition.x = _textPosition.x + textSize.x + _iconSpacing.get();
                    _iconPosition.y = contentArea._top + (contentArea._height - iconSize.y) / 2.0f;
                    _textPosition.y = contentArea._top + (contentArea._height - textSize.y) / 2.0f;
                    break;

                case Theme::Location::kBottom:
                    _iconPosition.x = contentArea._left + (contentArea._width - iconSize.x) / 2.0f;
                    _textPosition.x = contentArea._left + (contentArea._width - textSize.x) / 2.0f;
                    _textPosition.y = contentArea._top + (contentArea._height - totalHeight) / 2.0f;
                    _iconPosition.y = _textPosition.y + textSize.y + _iconSpacing.get();
                    break;
            }

            text->setContentPosition(_textPosition.x + offset.x, _textPosition.y + offset.y);
            text->setClippingWindow(clippingWindow);
            icon->setContentPosition(_iconPosition.x + offset.x, _iconPosition.y + offset.y);
            icon->setClippingWindow(clippingWindow);
        }
    }

    void Button::setState(Button::State state)
    {
        if (state == _state)
            return;

        if (_state == State::kPressed)
        {
            if (auto const & text = _text.get())
            {
                text->setContentPosition(_textPosition.x, _textPosition.y);
            }

            if (auto const & icon = _icon.get())
            {
                icon->setContentPosition(_iconPosition.x, _iconPosition.y);
            }
        }

        _state = state;

       if (_state == State::kPressed)
        {
            if (auto const & text = _text.get())
            {
                glm::vec2 textPosition = _textPosition + _pressOffset.get();
                text->setContentPosition(textPosition.x, textPosition.y);
            }

            if (auto const & icon = _icon.get())
            {
                glm::vec2 iconPosition = _iconPosition + _pressOffset.get();
                icon->setContentPosition(iconPosition.x, iconPosition.y);
            }
        }

        invalidateContent();
    }

    void Button::handle(models::checkable::OnCheckedChanged const & e)
    {
        setState(checkable() && checked() ? State::kPressed
                                          : (isHovered() ? State::kHovered
                                                         : State::kNormal));
        Checkable::handle(e);
    }

    void Button::handle(minire::events::application::OnMouseDown const & e)
    {
        if (e._mouseButton == minire::models::MouseButton::kLeft)
        {
            if (checkable())
            {
                if (!checked() || canUncheck())
                {
                    setState(checked() ? (isHovered() ? State::kHovered
                                                      : State::kNormal)
                                       : State::kPressed);
                }
            }
            else
            {
                setState(State::kPressed);
            }
        }
        CommonCallbacks::handle(e);
    }

    void Button::handle(gui::events::OnDragEnd const & e)
    {
        setState(checkable() && checked() ? State::kPressed
                                          : (isHovered() ? State::kHovered
                                                         : State::kNormal));
        CommonCallbacks::handle(e);
    }

    void Button::handle(gui::events::OnMouseEnter const & e)
    {
        bool isClickReturn = e._isClickReturn;

        if (checkable())
        {
            isClickReturn ^= checked();
        }
        setState(isClickReturn ? State::kPressed
                               : State::kHovered);
        CommonCallbacks::handle(e);
    }

    void Button::handle(gui::events::OnMouseLeave const & e)
    {
        if (!isDragging())
        {
            setState(checkable() && checked() ? State::kPressed
                                              : State::kNormal);
        }
        CommonCallbacks::handle(e);
    }

    void Button::handle(gui::events::OnClick const & e)
    {
        if (checkable())
            toggleCheck();

        setState(checkable() && checked() ? State::kPressed
                                          : (isHovered() ? State::kHovered
                                                         : State::kNormal));
        CommonCallbacks::handle(e);
    }
}
