#pragma once

#include <minire/gui/content-view.hpp>

#include <optional>

namespace minire { class GuiController; }

namespace minire::gui_controller
{
    class TextViewImpl
        : public gui::TextView
    {
    public:
        TextViewImpl(text::FormattedString const & text,
                     content::Id const & fontFace,
                     GuiController & controller);

        ~TextViewImpl() override;

        void initialize() override;

        std::pair<glm::vec2, bool> measure() const override;

        void setContentPosition(float left, float top) override;

        void setContentSize(float width, float height) override;

        void setContentArea(gui::Area const &) override;

        void setClippingWindow(gui::MaybeArea const &) override;

        size_t onZOrderChanged(size_t zOffset) override;

        void setVisible(bool visible) override;

        void setContent(content::Id const & fontFace,
                        text::FormattedString const & text) override;

        void setText(text::FormattedString const & text) override;

        void setFontFace(content::Id const & fontFace) override;

        void commit() override;

    private:
        void enqueueUncommited();

    private:
        GuiController       & _controller;

        // current state
        std::string           _labelId;
        content::Id           _fontFace;
        text::FormattedString _text;
        glm::vec2             _contentPosition{0, 0};
        glm::vec2             _contentSize{0, 0};
        glm::vec2             _textSize{0, 0};
        utils::MaybeRect      _clippingWindow = std::nullopt;
        size_t                _zOrder = 0;
        bool                  _visible = true;

        // pended updates
        std::optional<content::Id>           _newFontFace;
        std::optional<text::FormattedString> _newText;
        std::optional<glm::vec2>             _newContentPosition;
        std::optional<glm::vec2>             _newContentSize;
        std::optional<utils::MaybeRect>      _newClippingWindow;
        std::optional<size_t>                _newZOrder;
        std::optional<bool>                  _newVisible;
    };
}
