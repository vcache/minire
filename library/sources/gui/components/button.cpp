#include <minire/gui/components/button.hpp>

#include <minire/errors.hpp>

namespace minire::gui::components
{
    // TODO: clipping for an Icon
    // TODO: clipping for a Text
    Button::Button(std::string const & id,
                   Theme const & theme,
                   OverlayController & overlayController)
        : Component(id, theme, overlayController)
        , _bgNormal(*this, theme.button().makeNormalBg())
        , _bgHovered(*this, theme.button().makeHoveredBg())
        , _bgPressed(*this, theme.button().makePressedBg())
        , _text(*this)
        , _icon(*this)
        , _iconLocation(*this, theme.button().constants()._iconLocation)
        , _iconSpacing(*this, theme.button().constants()._iconSpacing)
        , _pressOffset(*this, theme.button().constants()._pressOffset)
    {
        padding() = theme.button().constants()._padding;
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
                                     Area const & clientArea)
    {
        // update content invalidator
        actualize(_bgNormal);
        actualize(_bgHovered);
        actualize(_bgPressed);
        actualize(_text);
        actualize(_icon);

        // revalidate visibility
        ImageView::Sptr const & background = activeBackground();
        if (_bgNormal.get()) (*_bgNormal)->setVisible((_bgNormal.get() == background) && effectiveVisible);
        if (_bgHovered.get()) (*_bgHovered)->setVisible((_bgHovered.get() == background) && effectiveVisible);
        if (_bgPressed.get()) (*_bgPressed)->setVisible((_bgPressed.get() == background) && effectiveVisible);

        if (_icon.get())
        {
            (*_icon)->setVisible(effectiveVisible);
        }

        if (_text.get())
        {
            (*_text)->setVisible(effectiveVisible);
        }

        // revalidate positions
        revalidatePositions(clientArea);

        // revalidate zOrder
        if (_bgNormal.get()) zOffset = (*_bgNormal)->onZOrderChanged(zOffset);
        if (_bgHovered.get()) zOffset = (*_bgHovered)->onZOrderChanged(zOffset);
        if (_bgPressed.get()) zOffset = (*_bgPressed)->onZOrderChanged(zOffset);
        if (_text.get()) zOffset = (*_text)->onZOrderChanged(zOffset);
        if (_icon.get()) zOffset = (*_icon)->onZOrderChanged(zOffset);

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
            assert(hasText);
            auto [w, h] = (*_text)->measure();
            return std::make_pair(w + extraWidth, h + extraHeight);
        }
        else if (!hasText)
        {
            // only an icon
            assert(hasIcon);
            auto [w, h] = (*_icon)->measure();
            return std::make_pair(w + extraWidth, h + extraHeight);
        }
        else
        {
            // an icon and a text
            assert(hasIcon);
            assert(hasText);
            auto [textWidth, textHeight] = (*_text)->measure();
            auto [iconWidth, iconHeight] = (*_icon)->measure();
            switch(_iconLocation.get())
            {
                case theme::Location::kLeft:
                case theme::Location::kRight:
                    return std::make_pair(iconWidth + textWidth + _iconSpacing.get() + extraWidth,
                                          std::max(iconHeight, textHeight) + extraHeight);

                case theme::Location::kTop:
                case theme::Location::kBottom:
                    return std::make_pair(std::max(iconWidth, textWidth) + extraWidth,
                                          iconHeight + textHeight + _iconSpacing.get() + extraHeight);
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

    void Button::revalidatePositions(Area const & area)
    {
        if (_bgNormal.get()) (*_bgNormal)->setContentArea(area);
        if (_bgHovered.get()) (*_bgHovered)->setContentArea(area);
        if (_bgPressed.get()) (*_bgPressed)->setContentArea(area);

        bool const hasIcon = _icon.get().operator bool();
        bool const hasText = _text.get().operator bool();

        glm::vec2 const offset = _state == State::kPressed ? _pressOffset.get()
                                                           : glm::vec2();

        if (!hasIcon && !hasText)
        {
            // nothing to do
        }
        else if (!hasIcon)
        {
            // only a text
            assert(hasText);
            auto [w, h] = (*_text)->measure();
            _textPosition.x = area._left + (area._width - w) / 2.0f;
            _textPosition.y = area._top + (area._height - h) / 2.0f;
            (*_text)->setContentPosition(_textPosition.x + offset.x, _textPosition.y + offset.y);
        }
        else if (!hasText)
        {
            // only an icon
            assert(hasIcon);
            auto [w, h] = (*_icon)->measure();
            _iconPosition.x = area._left + (area._width - w) / 2.0f;
            _iconPosition.y = area._top + (area._height - h) / 2.0f;
            (*_icon)->setContentPosition(_iconPosition.x + offset.x, _iconPosition.y + offset.y);
        }
        else
        {
            // an icon and a text
            assert(hasIcon);
            assert(hasText);
            auto [textWidth, textHeight] = (*_text)->measure();
            auto [iconWidth, iconHeight] = (*_icon)->measure();

            float const totalWidth = iconWidth + textWidth + _iconSpacing.get();
            float const totalHeight = iconHeight + textHeight + _iconSpacing.get();

            switch(_iconLocation.get())
            {
                case theme::Location::kLeft:
                    _iconPosition.x = area._left + (area._width - totalWidth) / 2.0f;
                    _textPosition.x = _iconPosition.x + iconWidth + _iconSpacing.get();
                    _iconPosition.y = area._top + (area._height - iconHeight) / 2.0f;
                    _textPosition.y = area._top + (area._height - textHeight) / 2.0f;
                    break;

                case theme::Location::kTop:
                    _iconPosition.x = area._left + (area._width - iconWidth) / 2.0f;
                    _textPosition.x = area._left + (area._width - textWidth) / 2.0f;
                    _iconPosition.y = area._top + (area._height - totalHeight) / 2.0f;
                    _textPosition.y = _iconPosition.y + iconHeight + _iconSpacing.get();
                    break;

                case theme::Location::kRight:
                    _textPosition.x = area._left + (area._width - totalWidth) / 2.0f;
                    _iconPosition.x = _textPosition.x + textWidth + _iconSpacing.get();
                    _iconPosition.y = area._top + (area._height - iconHeight) / 2.0f;
                    _textPosition.y = area._top + (area._height - textHeight) / 2.0f;
                    break;

                case theme::Location::kBottom:
                    _iconPosition.x = area._left + (area._width - iconWidth) / 2.0f;
                    _textPosition.x = area._left + (area._width - textWidth) / 2.0f;
                    _textPosition.y = area._top + (area._height - totalHeight) / 2.0f;
                    _iconPosition.y = _textPosition.y + textHeight + _iconSpacing.get();
                    break;
            }

            (*_text)->setContentPosition(_textPosition.x + offset.x, _textPosition.y + offset.y);
            (*_icon)->setContentPosition(_iconPosition.x + offset.x, _iconPosition.y + offset.y);
        }
    }

    void Button::setState(Button::State state)
    {
        if (state == _state)
            return;

        if (auto background = activeBackground(); background)
        {
            background->setVisible(false);
        }

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

        if (auto background = activeBackground(); background)
        {
            background->setVisible(visible().get());
        }

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
