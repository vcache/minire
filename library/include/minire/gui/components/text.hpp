#pragma once

#include <minire/content/id.hpp>
#include <minire/gui/component.hpp>
#include <minire/label.hpp>
#include <minire/text/formatted-string.hpp>

namespace minire::utils { class TextLayout; }

namespace minire::gui::components
{
    class Text
        : public Component
    {
    public:
        using Sptr = std::shared_ptr<Text>;
        using Wptr = std::weak_ptr<Text>;

        static constexpr std::string kName = "Text";

        // NOTE: If \a fontFace isn't provided, it will be taken from
        //       from Theme with a given Style.
        explicit Text(std::string const & id,
                      Theme const & theme,
                      Theme::Style const & style,
                      OverlayController & overlayController,
                      text::FormattedString text = {},
                      std::optional<content::Id> const & fontFace = std::nullopt);

        ~Text() override;

        // TODO: make make chaned initilizers (like in FormattedString) ?
        //       Actually, just need to add a single method like Text & text(text::FormattedString &&)

        Property<text::FormattedString> const & text() const { return _text; }
        Property<text::FormattedString> & text() { return _text; }

        Property<content::Id> const & fontFace() const { return _fontFace; }
        Property<content::Id> & fontFace() { return _fontFace; }

        std::optional<glm::vec2> measureContent() const override;

        utils::TextLayout const & textLayout() const;

    protected:
        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

    private:
        using TextLayoutUptr = std::unique_ptr<utils::TextLayout>;

        // TODO: it duplicates Label (since Object doesn't have contentRevalidator),
        //       otherwise, _label::text() and _label::setText can be used
        Property<text::FormattedString>  _text;
        Property<content::Id>            _fontFace;
        Label::Sptr                      _label;
        mutable std::optional<glm::vec2> _contentSize;
        mutable TextLayoutUptr           _textLayout;
    };
}
