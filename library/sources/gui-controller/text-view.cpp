#include <gui-controller/text-view.hpp>

#include <minire/events/controller.hpp>
#include <minire/gui-controller.hpp>
#include <minire/logging.hpp>

#include <utils/uuid.hpp>

#include <cassert>

namespace minire::gui_controller
{
    using namespace events::controller;

    TextViewImpl::TextViewImpl(text::FormattedString const & text,
                               content::Id const & fontFace,
                               bool const enableClipping,
                               GuiController & controller)
        : _controller(controller)
        , _fontFace(fontFace)
        , _text(text)
        , _enableClipping(enableClipping)
    {
        _textSize = _controller.measure(_text, _fontFace);
    }

    TextViewImpl::~TextViewImpl()
    {
        if (!_labelId.empty())
        {
            _controller.enqueue<RemoveLabel>(_labelId);
        }
    }

    void TextViewImpl::initialize()
    {
        enqueueUncommited();
    }

    std::pair<float, float> TextViewImpl::measure() const
    {
        return std::make_pair(_textSize.x, _textSize.y);
    }

    void TextViewImpl::setContentPosition(float left, float top)
    {
        if (glm::vec2 const newPosition(left, top);
            newPosition != _contentPosition || _newContentPosition)
        {
            _newContentPosition = newPosition;
            enqueueUncommited();
        }
    }

    void TextViewImpl::setContentSize(float width, float height)
    {
        if (glm::vec2 const contentSize{width, height};
            contentSize != _contentSize || _newContentSize)
        {
            _newContentSize = contentSize;
            enqueueUncommited();
        }
    }

    void TextViewImpl::setContentArea(gui::Area const & area)
    {
        // TODO: merge into a single event
        setContentPosition(area._left, area._top);
        setContentSize(area._width, area._height);
    }

    size_t TextViewImpl::onZOrderChanged(size_t zOffset)
    {
        if (_zOrder != zOffset || _newZOrder)
        {
            _newZOrder = zOffset;
            enqueueUncommited();
        }
        return zOffset + 1;
    }

    void TextViewImpl::setVisible(bool visible)
    {
        if (visible != _visible || _newVisible)
        {
            _newVisible = visible;
            enqueueUncommited();
        }
    }

    void TextViewImpl::setContent(content::Id const & fontFace,
                                  text::FormattedString const & text,
                                  bool enableClipping)
    {
        setFontFace(fontFace);
        setText(text);
        setEnableClipping(enableClipping);
    }

    void TextViewImpl::setText(text::FormattedString const & text)
    {
        if (text != _text || _newText)
        {
            _newText = text;
            _textSize = _controller.measure(_text, _fontFace);
            invalidate();
            enqueueUncommited();
        }
    }

    void TextViewImpl::setFontFace(content::Id const & fontFace)
    {
        if (fontFace != _fontFace || _newFontFace)
        {
            _newFontFace = fontFace;
            _textSize = _controller.measure(_text, _fontFace);
            invalidate();
            enqueueUncommited();
        }
    }

    void TextViewImpl::setEnableClipping(bool const enableClipping)
    {
        if (enableClipping != _enableClipping || _newEnableClipping)
        {
            _newEnableClipping = enableClipping;
            enqueueUncommited();
        }
    }

    void TextViewImpl::setClippingArea()
    {
        if (_enableClipping)
        {
            _controller.enqueue<SetLabelClipping>(_labelId, _contentSize);
        }
        else
        {
            _controller.enqueue<SetLabelClipping>(_labelId, std::nullopt);
        }
    }

    void TextViewImpl::commit()
    {
        bool updateClippingArea = false;

        if (_labelId.empty())
        {
            _labelId = utils::newUuid();

            if (_newFontFace)
            {
                _fontFace = *_newFontFace;
                _newFontFace.reset();
            }

            if (_newText)
            {
                _text = *_newText;
                _newText.reset();
            }

            if (_newContentPosition)
            {
                _contentPosition = *_newContentPosition;
                _newContentPosition.reset();
            }

            if (_newContentSize)
            {
                _contentSize = *_newContentSize;
                _newContentSize.reset();
            }

            if (_newZOrder)
            {
                _zOrder = *_newZOrder;
                _newZOrder.reset();

            }

            if (_newVisible)
            {
                _visible = *_newVisible;
                _newVisible.reset();
            }

            if (_newEnableClipping)
            {
                _enableClipping = *_newEnableClipping;
                _newEnableClipping.reset();
            }

            _controller.enqueue<CreateLabel>(
                _labelId, _text, _fontFace, _contentPosition, _visible, _zOrder);
            updateClippingArea = true;
        }

        if (_newFontFace)
        {
            _fontFace = *_newFontFace;
            if (_fontFace != *_newFontFace)
            {
                assert(!_labelId.empty());
                _controller.enqueue<SetLabelFontFace>(_labelId, _fontFace);
            }
            _newFontFace.reset();
        }

        if (_newText)
        {
            _text = *_newText;
            if (_text != *_newText)
            {
                assert(!_labelId.empty());
                _controller.enqueue<SetLabelText>(_labelId, _text);
            }
            _newText.reset();
        }

        if (_newContentPosition)
        {
            if (_contentPosition != *_newContentPosition)
            {
                _contentPosition = *_newContentPosition;
                assert(!_labelId.empty());
                _controller.enqueue<MoveLabel>(_labelId, _contentPosition);
            }
            _newContentPosition.reset();
        }

        if (_newContentSize)
        {
            if (_contentSize != *_newContentSize)
            {
                _contentSize = *_newContentSize;
                updateClippingArea = true;
            }
            _newContentSize.reset();
        }

        if (_newZOrder)
        {
            if (_zOrder != *_newZOrder)
            {
                _zOrder = *_newZOrder;
                assert(!_labelId.empty());
                _controller.enqueue<SetLabelZOrder>(_labelId, _zOrder);
            }
            _newZOrder.reset();
        }

        if (_newVisible)
        {
            if (_visible != *_newVisible)
            {
                _visible = *_newVisible;
                assert(!_labelId.empty());
                _controller.enqueue<SetLabelVisible>(_labelId, _visible);
            }
            _newVisible.reset();
        }

        if (_newEnableClipping)
        {
            if (_enableClipping != *_newEnableClipping)
            {
                _enableClipping = *_newEnableClipping;
                updateClippingArea = true;
            }
            _newEnableClipping.reset();
        }

        if (updateClippingArea)
        {
            setClippingArea();
        }
    }

    void TextViewImpl::enqueueUncommited()
    {
        _controller.enqueueView(shared_from_this());
    }
}