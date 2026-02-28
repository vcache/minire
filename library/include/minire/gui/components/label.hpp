#pragma once

#include <minire/gui/components/text.hpp>
#include <minire/text/formatted-string.hpp>

namespace minire::gui::components
{
    class Label final : public Text
    {
    public:
        using Sptr = std::shared_ptr<Label>;
        using Wptr = std::weak_ptr<Label>;

        explicit Label(std::string const & id,
                       Theme const & theme,
                       Theme::Style const & style,
                       OverlayController & overlayController,
                       text::FormattedString const & text = {});

        Property<text::FormattedString> const & text() const { return _text; }
        Property<text::FormattedString> & text() { return _text; }

        // NOTE: shouldn't use content() directly, it won't be correctly revalidated.

    private:
        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

    private:
        Property<text::FormattedString> _text;
    };
}