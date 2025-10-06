#include <minire/gui/components/button.hpp>

#include <minire/basic-controller.hpp>
#include <minire/errors.hpp>
#include <minire/events/controller.hpp>
#include <minire/models/mouse-button.hpp>
#include <utils/overloaded.hpp>
#include <utils/uuid.hpp>

#include <cassert>

namespace minire::gui::components
{
    Button::Button(GuiController & controller,
                   std::string const & id,
                   std::shared_ptr<Container> const & parent,
                   Background const & background,
                   MaybeIcon const & icon,
                   MaybeText const & text,
                   Arrangers arrangers,
                   bool const checkable)
        : Component(controller, id, parent)
        , Checkable(checkable)
        , _background(background)
        , _icon(icon)
        , _text(text)
    {
        if (_icon)
        {
            utils::Patch patch;
            if (_icon->_rect) patch = *_icon->_rect;
            auto [size, resizable] = measure(patch, _icon->_texture);
            assert(!resizable);
            _iconSize = size;
        }

        if (_text)
        {
            _textSize = measure(_text->_text, _text->_fontFace);
        }

        // NOTE: setArrangers will call onContentAreaChanged, thus,
        //       sprite and labels IDs should be empty() at that time,
        //       in order to achieve actual contentArea.
        setArrangers(arrangers);

        {
            Area const & area = contentArea();

            _normalSprite = utils::newUuid();
            _hoveredSprite = utils::newUuid();
            _pressedSprite = utils::newUuid();

            enqueue<events::controller::CreateSprite>(
                _normalSprite, _background._texture, _background._normal,
                glm::vec2(area._left, area._top), glm::vec2(area._width, area._height),
                visible() && _state == State::kNormal, zOrder());

            enqueue<events::controller::CreateSprite>(
                _hoveredSprite, _background._texture, _background._hovered,
                glm::vec2(area._left, area._top), glm::vec2(area._width, area._height),
                visible() && _state == State::kHovered, zOrder());

            enqueue<events::controller::CreateSprite>(
                _pressedSprite, _background._texture, _background._pressed,
                glm::vec2(area._left, area._top), glm::vec2(area._width, area._height),
                visible() && _state == State::kPressed, zOrder());
        }

        if (_icon)
        {
            _iconSprite = utils::newUuid();
            utils::Patch patch;
            if (_icon->_rect) patch = *_icon->_rect;
            enqueue<events::controller::CreateSprite>(
                _iconSprite, _icon->_texture, patch, _iconPosition,
                glm::vec2(_iconSize.x, _iconSize.y), visible(), zOrder());

            // TODO: clipping for an Icon
        }

        if (_text)
        {
            _textLabel = utils::newUuid();
            enqueue<events::controller::CreateLabel>(
                _textLabel, _text->_text, _text->_fontFace, _textPosition,
                visible(), zOrder());

            // TODO: SetLabelClipping
        }
    }

    void Button::onContentAreaChanged()
    {
        Area const & area = contentArea();

        for(std::string const & spriteId : {_normalSprite,
                                            _hoveredSprite,
                                            _pressedSprite})
        {
            if (spriteId.empty())
                continue;

            // TODO: merge into Move+Resize
            enqueue<events::controller::MoveSprite>(
                spriteId, glm::vec2(area._left, area._top)
            );
            enqueue<events::controller::ResizeSprite>(
                spriteId, glm::vec2(area._width, area._height)
            );
        }

        if (!_icon && !_text)
        {
            // nothing to todo
        }
        else if (!_icon)
        {
            // only text
            assert(_text);
            _textPosition.x = area._left + (area._width - _textSize.x) / 2.0f;
            _textPosition.y = area._top + (area._height - _textSize.y) / 2.0f;
            if (!_textLabel.empty())
            {
                enqueue<events::controller::MoveLabel>(_textLabel, _textPosition);
            }
        }
        else if (!_text)
        {
            // only icon
            assert(_icon);
            _iconPosition.x = area._left + (area._width - _iconSize.x) / 2.0f;
            _iconPosition.y = area._top + (area._height - _iconSize.y) / 2.0f;
            if (!_iconSprite.empty())
            {
                enqueue<events::controller::MoveSprite>(_iconSprite, _iconPosition);
            }
        }
        else
        {
            // an icon and a text
            assert(_icon);
            assert(_text);
            glm::vec2 combinedSize = _iconSize + _textSize + _icon->_spacing; // TODO: cache it
            switch(_icon->_position)
            {
                case Icon::Position::kLeft:
                    _iconPosition.x = area._left + (area._width - combinedSize.x) / 2.0f;
                    _textPosition.x = _iconPosition.x + _iconSize.x + _icon->_spacing;
                    _iconPosition.y = area._top + (area._height - _iconSize.y) / 2.0f;
                    _textPosition.y = area._top + (area._height - _textSize.y) / 2.0f;
                    break;

                case Icon::Position::kTop:
                    _iconPosition.x = area._left + (area._width - _iconSize.x) / 2.0f;
                    _textPosition.x = area._left + (area._width - _textSize.x) / 2.0f;
                    _iconPosition.y = area._top + (area._height - combinedSize.y) / 2.0f;
                    _textPosition.y = _iconPosition.y + _iconSize.y + _icon->_spacing;
                    break;

                case Icon::Position::kRight:
                    _textPosition.x = area._left + (area._width - combinedSize.x) / 2.0f;
                    _iconPosition.x = _textPosition.x + _textSize.x + _icon->_spacing;
                    _iconPosition.y = area._top + (area._height - _iconSize.y) / 2.0f;
                    _textPosition.y = area._top + (area._height - _textSize.y) / 2.0f;
                    break;

                case Icon::Position::kBottom:
                    _iconPosition.x = area._left + (area._width - _iconSize.x) / 2.0f;
                    _textPosition.x = area._left + (area._width - _textSize.x) / 2.0f;
                    _textPosition.y = area._top + (area._height - combinedSize.y) / 2.0f;
                    _iconPosition.y = _textPosition.y + _textSize.y + _icon->_spacing;
                    break;
            }

            if (!_textLabel.empty())
            {
                enqueue<events::controller::MoveLabel>(_textLabel, _textPosition);
            }

            if (!_iconSprite.empty())
            {
                enqueue<events::controller::MoveSprite>(_iconSprite, _iconPosition);
            }
        }
    }

    std::optional<std::pair<float, float>> Button::measureContent() const
    {
        float const extraWidth = _contentMargin._left + _contentMargin._right;
        float const extraHeight = _contentMargin._top + _contentMargin._bottom;

        if (!_icon && !_text)
        {
            return std::make_pair(extraWidth, extraHeight);
        }
        else if (!_icon)
        {
            // only text
            assert(_text);
            return std::make_pair(_textSize.x + extraWidth,
                                  _textSize.y + extraHeight);
        }
        else if (!_text)
        {
            // only icon
            assert(_icon);
            return std::make_pair(_iconSize.x + extraWidth,
                                  _iconSize.y + extraHeight);
        }
        else
        {
            // an icon and a text
            assert(_icon);
            assert(_text);
            switch(_icon->_position)
            {
                case Icon::Position::kLeft:
                case Icon::Position::kRight:
                    return std::make_pair(_iconSize.x + _textSize.x + _icon->_spacing + extraWidth,
                                          std::max(_iconSize.y, _textSize.y) + extraHeight);

                case Icon::Position::kTop:
                case Icon::Position::kBottom:
                    return std::make_pair(std::max(_iconSize.x, _textSize.x) + extraWidth,
                                          _iconSize.y + _textSize.y + _icon->_spacing + extraHeight);
            }

            MINIRE_THROW("unknown icon position: {}", static_cast<int>(_icon->_position));
        }
    }

    void Button::setContentMargin(utils::Rect const & contentMargin)
    {
        _contentMargin = contentMargin;
        rearrange();
    }

    Button::~Button()
    {
        for(std::string const & spriteId : {_normalSprite,
                                            _hoveredSprite,
                                            _pressedSprite,
                                            _iconSprite})
        {
            if (spriteId.empty())
                continue;
            enqueue<events::controller::RemoveSprite>(spriteId);
        }

        if (!_textLabel.empty())
        {
            enqueue<events::controller::RemoveLabel>(_textLabel);
        }
    }

    void Button::onVisibleChanged()
    {
        if (std::string const & spriteId = activeBackground();
            !spriteId.empty())
        {
            enqueue<events::controller::SetSpriteVisible>(spriteId, visible());
        }

        if (!_iconSprite.empty())
        {
            enqueue<events::controller::SetSpriteVisible>(_iconSprite, visible());
        }

        if (!_textLabel.empty())
        {
            enqueue<events::controller::SetLabelVisible>(_textLabel, visible());
        }
    }

    size_t Button::onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                                   ZOrderUpdates & sprites)
    {
        if (!_normalSprite.empty()) sprites.emplace_back(_normalSprite, offset++);
        if (!_hoveredSprite.empty()) sprites.emplace_back(_hoveredSprite, offset++);
        if (!_pressedSprite.empty()) sprites.emplace_back(_pressedSprite, offset++);
        if (!_iconSprite.empty()) sprites.emplace_back(_iconSprite, offset++);
        if (!_textLabel.empty()) labels.emplace_back(_textLabel, offset++);
        return offset;
    }

    bool Button::handle(events::application::OnMouseDown const & e)
    {
        if (e._mouseButton == minire::models::MouseButton::kLeft)
        {
            if (checkable())
            {
                if (checked() && !canUncheck())
                    return true;

                setState(checked() ? (isHovered() ? State::kHovered
                                                  : State::kNormal)
                                   : State::kPressed);
            }
            else
            {
                setState(State::kPressed);
            }
        }
        return true;
    }

    bool Button::handle(events::application::OnMouseWheel const & e)
    {
        if (_mouseWheelCallback)
        {
            _mouseWheelCallback(*this, e);
            return true;
        }
        return false;
    }

    void Button::onDragEnd(std::optional<events::application::OnMouseUp> const &)
    {
        setState(checkable() && checked() ? State::kPressed
                                          : (isHovered() ? State::kHovered
                                                         : State::kNormal));
    }

    void Button::onMouseEnter(bool isClickReturn)
    {
        if (checkable())
        {
            isClickReturn ^= checked();
        }
        setState(isClickReturn ? State::kPressed
                               : State::kHovered);
    }

    void Button::onMouseLeave()
    {
        if (!isDragging())
        {
            setState(checkable() && checked() ? State::kPressed
                                              : State::kNormal);
        }
    }

    void Button::onClick()
    {
        if (checkable())
            toggleCheck();

        if (_clickCallback)
            _clickCallback(*this);

        setState(checkable() && checked() ? State::kPressed
                                          : (isHovered() ? State::kHovered
                                                         : State::kNormal));
    }

    void Button::onCheckChanged()
    {
        setState(checkable() && checked() ? State::kPressed
                                          : (isHovered() ? State::kHovered
                                                         : State::kNormal));
    }

    void Button::setState(Button::State state)
    {
        if (state == _state)
            return;

        enqueue<events::controller::SetSpriteVisible>(
            activeBackground(), false);

        if (_state == State::kPressed)
        {
            if (!_textLabel.empty())
            {
                enqueue<events::controller::MoveLabel>(_textLabel, _textPosition);
            }

            if (!_iconSprite.empty())
            {
                enqueue<events::controller::MoveSprite>(_iconSprite, _iconPosition);
            }
        }

        _state = state;

        enqueue<events::controller::SetSpriteVisible>(
            activeBackground(), visible());

        if (_state == State::kPressed)
        {
            if (!_textLabel.empty())
            {
                enqueue<events::controller::MoveLabel>(_textLabel,
                                                       _textPosition + _pressedContentDelta);
            }

            if (!_iconSprite.empty())
            {
                enqueue<events::controller::MoveSprite>(_iconSprite,
                                                        _iconPosition + _pressedContentDelta);
            }
        }
    }

    std::string const & Button::activeBackground() const
    {
        switch(_state)
        {
            case State::kNormal:  return _normalSprite;
            case State::kHovered: return _hoveredSprite;
            case State::kPressed: return _pressedSprite;
        }
        MINIRE_THROW("unknown button state: {}", static_cast<int>(_state));
    }
}
