#include <minire/gui/components/button.hpp>

#include <minire/errors.hpp>
#include <minire/gui/layout.hpp>
#include <minire/gui/layouts/array.hpp>

namespace minire::gui::components
{
    Button::Button(std::string const & id,
                   Theme const & theme,
                   Theme::Style const & style,
                   OverlayController & overlayController,
                   bool const hasText,
                   bool const hasIcon)
        : Component(id, theme, style, overlayController)
        , _bgNormal(std::make_shared<components::Image>(
                "__bg-normal__", theme, style, overlayController,
                theme.get<minire::models::sprite::MaybeImage>(kName, "bg-normal", style)))
        , _bgHovered(std::make_shared<components::Image>(
                "__bg-hovered__", theme, style, overlayController,
                theme.get<minire::models::sprite::MaybeImage>(kName, "bg-hovered", style)))
        , _bgPressed(std::make_shared<components::Image>(
                "__bg-pressed__", theme, style, overlayController,
                theme.get<minire::models::sprite::MaybeImage>(kName, "bg-pressed", style)))
        , _content(hasIcon || hasText ? std::make_shared<Component>("__content__", theme, style,
                                                                    overlayController)
                                      : Component::Sptr{})
        , _text(hasText ? std::make_shared<components::Text>("__caption__", theme, concat(style, kName),
                                                             overlayController)
                        : components::Text::Sptr{})
        , _icon(hasIcon ? std::make_shared<components::Image>("__icon__", theme, concat(style, kName),
                                                              overlayController, std::nullopt)
                        : components::Image::Sptr{})
        , _iconLocation(*this, theme.get<Theme::Location>(kName, "icon-location", style))
        , _iconSpacing(*this, theme.get<float>(kName, "icon-spacing", style))
        , _pressOffset(*this, theme.get<glm::vec2>(kName, "press-offset", style))
        , _contentPadding(*this, theme.get<utils::Rect>(kName, "content-padding", style))
        , _hasText(hasText)
        , _hasIcon(hasIcon)
    {
        isDraggable() = true;
    }

    void Button::initialize()
    {
        auto sharedThis = shared_from_this();

        assert(_bgNormal);
        _bgNormal->setParent(sharedThis);
        _bgNormal->setEventTransparent(true);

        assert(_bgHovered);
        _bgHovered->setParent(sharedThis);
        _bgHovered->setEventTransparent(true);

        assert(_bgPressed);
        _bgPressed->setParent(sharedThis);
        _bgPressed->setEventTransparent(true);

        if (_content)
        {
            _content->setParent(sharedThis);
            _content->setEventTransparent(true);
        }

        if(_text)
        {
            assert(_content);
            _text->setParent(_content);
            _text->setEventTransparent(true);
            _text->horizontal() = _text->vertical() =
                Arranger(position::Center{}, dimension::Content{});
        }

        if(_icon)
        {
            assert(_content);
            _icon->setParent(_content);
            _icon->setEventTransparent(true);
            _icon->horizontal() = _icon->vertical() =
                Arranger(position::Center{}, dimension::Content{});
        }

        rebuildLayout();
        updateArrangers();
        updateBackground();
    }

    size_t Button::revalidateContent(size_t zOffset,
                                     bool const /*effectiveVisible*/,
                                     Area const & /*contentArea*/,
                                     Area const & /*clippingWindow*/)
    {
        if (_iconLocation.isInvalidated() ||
            _iconSpacing.isInvalidated() ||
            _contentPadding.isInvalidated())
        {
            rebuildLayout();
            updateArrangers();
        }

        if (_pressOffset.isInvalidated())
        {
            updateArrangers();
        }

        _iconLocation.revalidate();
        _iconSpacing.revalidate();
        _pressOffset.revalidate();
        _contentPadding.revalidate();

        return zOffset;
    }

    void Button::rebuildLayout()
    {
        if (_hasIcon && _hasText)
        {
            // an icon and a text
            assert(_content);
            assert(_hasIcon && _icon);
            assert(_hasText && _text);
            switch(_iconLocation.get())
            {
                case Theme::Location::kLeft:
                    _content->newLayout<layouts::Row>()->pushBack(_icon, dimension::Content{})
                                                        .pushBack(dimension::Constant{_iconSpacing.get()})
                                                        .pushBack(_text, dimension::Fill{});
                    break;

                case Theme::Location::kTop:
                    _content->newLayout<layouts::Column>()->pushBack(_icon, dimension::Content{})
                                                           .pushBack(dimension::Constant{_iconSpacing.get()})
                                                           .pushBack(_text, dimension::Fill{});
                    break;

                case Theme::Location::kRight:
                    _content->newLayout<layouts::Row>()->pushBack(_text, dimension::Fill{})
                                                        .pushBack(dimension::Constant{_iconSpacing.get()})
                                                        .pushBack(_icon, dimension::Content{});
                    break;

                case Theme::Location::kBottom:
                    _content->newLayout<layouts::Column>()->pushBack(_text, dimension::Fill{})
                                                           .pushBack(dimension::Constant{_iconSpacing.get()})
                                                           .pushBack(_icon, dimension::Content{});
                    break;
            }
        }
        else if (_content)
        {
            _content->newLayout<LinearLayout>();
        }
    }

    void Button::updateArrangers()
    {
        if(_content)
        {
            bool const displaced = State::kPressed == _state;
            std::optional<glm::vec2> size = measureContent();

            assert(size);
            _content->horizontal() = Arranger(position::Center{},
                                              dimension::Constant{size->x},
                                              displaced ? _pressOffset.get().x : 0);
            _content->vertical() = Arranger(position::Center{},
                                            dimension::Constant{size->y},
                                            displaced ? _pressOffset.get().y : 0);
        }
    }

    void Button::updateBackground()
    {
        assert(_bgNormal);
        _bgNormal->visible()  = State::kNormal == _state;

        assert(_bgHovered);
        _bgHovered->visible() = State::kHovered == _state;

        assert(_bgPressed);
        _bgPressed->visible() = State::kPressed == _state;
    }

    // TODO: it should be calculated automatically by the Layout
    // TODO: cache this value
    std::optional<glm::vec2> Button::measureContent() const
    {
        float const extraWidth = _contentPadding.get()._left + _contentPadding.get()._right;
        float const extraHeight = _contentPadding.get()._top + _contentPadding.get()._bottom;

        if (!_hasIcon && !_hasText)
        {
            assert(!_text && !_icon);
            return glm::vec2(extraWidth, extraHeight);
        }
        else if (!_hasIcon)
        {
            // only a text
            assert(_hasText && _text && !_icon);
            auto const & size = _text->measureContent();
            assert(size);
            return glm::vec2(size->x + extraWidth, size->y + extraHeight);
        }
        else if (!_hasText)
        {
            // only an icon
            assert(_hasIcon && !_text && _icon);
            auto const & size = _icon->measureContent();
            assert(size);
            return glm::vec2(size->x + extraWidth, size->y + extraHeight);
        }
        else
        {
            // an icon and a text
            assert(_hasIcon && _icon);
            assert(_hasText && _text);
            auto const & textSize = _text->measureContent();
            auto const & iconSize = _icon->measureContent();
            assert(textSize && iconSize);
            switch(_iconLocation.get())
            {
                case Theme::Location::kLeft:
                case Theme::Location::kRight:
                    return glm::vec2(iconSize->x + textSize->x + _iconSpacing.get() + extraWidth,
                                     std::max(iconSize->y, textSize->y) + extraHeight);

                case Theme::Location::kTop:
                case Theme::Location::kBottom:
                    return glm::vec2(std::max(iconSize->x, textSize->x) + extraWidth,
                                     iconSize->y + textSize->y + _iconSpacing.get() + extraHeight);
            }

            MINIRE_THROW("unknown icon position: {}", static_cast<int>(_iconLocation.get()));
        }
    }

    void Button::setState(Button::State state)
    {
        if (state == _state)
            return;

        _state = state;

        updateArrangers();
        updateBackground();
    }

    void Button::handle(models::checkable::OnCheckedChanged const & e)
    {
        setState(checkable() && checked() ? State::kPressed
                                          : (isHovered() ? State::kHovered
                                                         : State::kNormal));
        Checkable::handle(e);
    }

    void Button::handle(application::OnMouseDown const & e)
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

    void Button::handle(gui::OnDragEnd const & e)
    {
        setState(checkable() && checked() ? State::kPressed
                                          : (isHovered() ? State::kHovered
                                                         : State::kNormal));
        CommonCallbacks::handle(e);
    }

    void Button::handle(gui::OnMouseEnter const & e)
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

    void Button::handle(gui::OnMouseLeave const & e)
    {
        if (!isDragging())
        {
            setState(checkable() && checked() ? State::kPressed
                                              : State::kNormal);
        }
        CommonCallbacks::handle(e);
    }

    void Button::handle(gui::OnClick const & e)
    {
        if (checkable())
            toggleCheck();

        setState(checkable() && checked() ? State::kPressed
                                          : (isHovered() ? State::kHovered
                                                         : State::kNormal));
        CommonCallbacks::handle(e);
    }
}
