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
                               GuiController & controller)
        : _controller(controller)
        , _fontFace(fontFace)
        , _text(text)
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

    std::pair<glm::vec2, bool> TextViewImpl::measure() const
    {
        return std::make_pair(_textSize, false);
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

    void TextViewImpl::setClippingWindow(gui::MaybeArea const & area)
    {
        utils::MaybeRect rect;
        if (area)
        {
            rect.emplace(area->_left,
                         area->_top,
                         area->_left + area->_width,
                         area->_top + area->_height);
        }

        if (rect != _clippingWindow)
        {
            _newClippingWindow = rect;
            enqueueUncommited();
        }
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
                                  text::FormattedString const & text)
    {
        setFontFace(fontFace);
        setText(text);
    }

    void TextViewImpl::setText(text::FormattedString const & text)
    {
        if (text != _text || _newText)
        {
            _newText = text;
            _textSize = _controller.measure(text, _fontFace);
            _textLayout.reset();
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
            _textLayout.reset();
            invalidate();
            enqueueUncommited();
        }
    }

    utils::TextLayout const & TextViewImpl::textLayout() const
    {
        if (!_textLayout)
        {
            _textLayout = _controller.layout(_newText ? *_newText : _text,
                                             _fontFace);
        }
        return *_textLayout;
    }

    void TextViewImpl::commit()
    {
        bool updateClippingWindow = false;

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

            if (_newClippingWindow)
            {
                _clippingWindow = *_newClippingWindow;
                _newClippingWindow.reset();
            }

            _controller.enqueue<CreateLabel>(
                _labelId, _text, _fontFace, _contentPosition, _visible, _zOrder);
            updateClippingWindow = true;
        }

        if (_newFontFace)
        {
            if (_fontFace != *_newFontFace)
            {
                _fontFace = *_newFontFace;
                assert(!_labelId.empty());
                _controller.enqueue<SetLabelFontFace>(_labelId, _fontFace);
            }
            _newFontFace.reset();
        }

        if (_newText)
        {
            if (_text != *_newText)
            {
                _text = *_newText;
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
                updateClippingWindow = true;
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

        if (_newClippingWindow)
        {
            if (_clippingWindow != *_newClippingWindow)
            {
                _clippingWindow = *_newClippingWindow;
                updateClippingWindow = true;
            }
            _newClippingWindow.reset();
        }

        if (updateClippingWindow)
        {
            assert(!_labelId.empty());
            _controller.enqueue<SetLabelClippingWindow>(_labelId, _clippingWindow);
        }
    }

    void TextViewImpl::enqueueUncommited()
    {
        _controller.enqueueView(shared_from_this());
    }
}
