#pragma once

#include <minire/content/id.hpp>
#include <minire/gui/component.hpp>

namespace minire::text { class FormattedString; }

namespace minire::gui::components
{
    class Text final
        : public Component
    {
    public:
        explicit Text(GuiController & controller,
                      std::string const & id,
                      std::shared_ptr<Container> const & parent,
                      text::FormattedString const & text,
                      content::Id const & fontFace,
                      Arrangers arrangers = Arrangers());
        ~Text() override;

    public:
        void setText(text::FormattedString const & text);

    private:
        void onVisibleChanged() override;
        void onContentAreaChanged() override;
        size_t onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                               ZOrderUpdates & sprites) override;
        std::optional<std::pair<float, float>> measureContent() const override;

    private:
        std::string _labelId;
        content::Id _fontFace;
        glm::vec2   _measurements;
        bool        _clippingSet;
    };
}
