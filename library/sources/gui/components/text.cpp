#include <minire/gui/components/text.hpp>

#include <minire/gui/overlay-controller.hpp>
#include <minire/models/label.hpp>
#include <minire/utils/glyph-grid.hpp>

#include <cassert>

namespace minire::gui::components
{
    Text::Text(std::string const & id,
               Theme const & theme,
               Theme::Style const & style,
               OverlayController & overlayController,
               text::FormattedString text,
               std::optional<content::Id> const & fontFace)
        : Component(id, theme, style, overlayController)
        , _text(*this, std::move(text))
        , _fontFace(*this, fontFace ? *fontFace
                                    : theme.get<content::Id>("text", "font-face", style))
    {
        horizontal()->_dimension = dimension::Content{};
        vertical()->_dimension = dimension::Content{};
    }

    Text::~Text()
    {
        if (_label)
        {
            _label->detach();
        }
    }

    std::optional<glm::vec2> Text::measureContent() const
    {
        if (!_contentSize || _text.isInvalidated() || _fontFace.isInvalidated())
        {
            _contentSize = overlayController().measure(_text.get(), _fontFace.get());
        }
        return *_contentSize;
    }

    utils::TextLayout const & Text::textLayout() const
    {
        if (!_textLayout || _text.isInvalidated() || _fontFace.isInvalidated())
        {
            _textLayout = overlayController().layout(_text.get(), _fontFace.get());
        }
        return *_textLayout;
    }

    // TODO: erase Label when text is empty?
    size_t Text::revalidateContent(size_t zOffset,
                                   bool const effectiveVisible,
                                   Area const & contentArea,
                                   Area const & clippingWindow)
    {
        glm::vec2 const newPosition(contentArea._left, contentArea._top);
        utils::Rect const newClippingWindow(
            clippingWindow._left,
            clippingWindow._top,
            clippingWindow._left + clippingWindow._width,     // TODO: +1 ?
            clippingWindow._top + clippingWindow._height);    // TODO: +1 ?

        if (!_label)
        {
            _label = overlayController().create(models::Label
            {
                ._text = _text.get(),
                ._fontFace = _fontFace.get(),
                ._position = newPosition,
                ._clippingWindow = newClippingWindow,
                ._zOrder = zOffset,
                ._visible = effectiveVisible,
            });
        }
        else
        {
            // TODO: don't update these parameters while in an invisible state

            if (_text.isInvalidated())
            {
                _label->setText(_text.get());
                invalidateContent();
                _contentSize.reset();
                _textLayout.reset();
            }
            
            if (_fontFace.isInvalidated())
            {
                _label->setFontFace(_fontFace.get());
                invalidateContent();
                _contentSize.reset();
                _textLayout.reset();
            }

            _label->setPosition(newPosition);
            _label->setClippingWindow(newClippingWindow);
            _label->setZOrder(zOffset);
            _label->setVisible(effectiveVisible);
        }

        _text.revalidate();
        _fontFace.revalidate();

        return zOffset + 1;
    }
}
