#include <minire/gui/components/label.hpp>

#include <cassert>

namespace minire::gui::components
{
    Label::Label(std::string const & id,
                   Theme const & theme,
                   Theme::Style const & style,
                   OverlayController & overlayController,
                   text::FormattedString const & text)
        : Text(id, theme, style, overlayController,
               theme.makeText("label", "", style, text) )
        , _text(*this, text)
    {}

    size_t Label::revalidateContent(size_t zOffset,
                                    bool const effectiveVisible,
                                    Area const & contentArea,
                                    Area const & clippingWindow)
    {
        if (_text.isInvalidated())
        {
            content().setText(_text.get());
            _text.revalidate();
        }

        return Text::revalidateContent(
            zOffset, effectiveVisible, contentArea, clippingWindow);
    }
}