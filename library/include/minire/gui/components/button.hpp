#pragma once

#include <minire/gui/component.hpp>
#include <minire/gui/models/checkable.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace minire::gui::components
{
    class Button final
        : public Component
        , public models::Checkable
    {
        enum State
        {
            kNormal, kHovered, kPressed,
        };

    public:
        // TODO: these items will be the same across
        //       various instances of a Button, thus,
        //       they can be cached and re-used.
        struct Background
        {
            content::Id      _texture;
            utils::NinePatch _normal;
            utils::NinePatch _hovered;
            utils::NinePatch _pressed;
            // TODO: focused, hovered while pressed, and etc
        };

        struct Icon
        {
            enum class Position
            {
                kLeft, kTop, kRight, kBottom,
            };

            content::Id      _texture;
            utils::MaybeRect _rect;
            float            _spacing = 5;
            Position         _position = Position::kLeft;
        };

        using MaybeIcon = std::optional<Icon>;

        struct Text
        {
            content::Id           _fontFace;
            text::FormattedString _text;
        };

        using MaybeText = std::optional<Text>;

    public:
        using Sptr = std::shared_ptr<Button>;
        using Wptr = std::weak_ptr<Button>;

        Button(GuiController & controller,
               std::string const & id,
               std::shared_ptr<Container> const & parent,
               Background const & background,
               MaybeIcon const & icon = std::nullopt,
               MaybeText const & text = std::nullopt,
               Arrangers arrangers = Arrangers(),
               bool const checkable = false);

        ~Button() override;

        utils::Rect const & contentMargin() const { return _contentMargin; }
        void setContentMargin(utils::Rect const &);

        glm::vec2 const & pressedContentDelta() const { return _pressedContentDelta; }
        void setPressedContentDelta(glm::vec2 const & v) {_pressedContentDelta = v; }

    public:
        using ClickCallback = std::function<void(Button &)>;

        template<typename Callback>
        void setClickCallback(Callback clickCallback)
        {
            _clickCallback = clickCallback;
        }

        bool hasClickCallback() const { return _clickCallback.operator bool(); }

    public:
        using MouseWheelCallback =
            std::function<void(Button &, events::application::OnMouseWheel const &)>;

        template<typename Callback>
        void setMouseWheelCallback(Callback callback)
        {
            _mouseWheelCallback = callback;
        }

        bool hasMouseWheelCallback() const { return _mouseWheelCallback.operator bool(); }

    private:
        void onVisibleChanged() override;
        void onContentAreaChanged() override;
        size_t onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                               ZOrderUpdates & sprites) override;
        std::optional<std::pair<float, float>> measureContent() const override;

        void onCheckChanged() override;

        bool handle(events::application::OnMouseDown const &) override;
        bool handle(events::application::OnMouseWheel const &) override;

        void onDragEnd(std::optional<events::application::OnMouseUp> const &) override;
        void onMouseEnter(bool isClickReturn) override;
        void onMouseLeave() override;
        void onClick() override;

        void setState(State);

        std::string const & activeBackground() const;

    private:
        Background         _background;
        MaybeIcon          _icon;
        MaybeText          _text;
        utils::Rect        _contentMargin = utils::Rect(0);
        ClickCallback      _clickCallback;
        MouseWheelCallback _mouseWheelCallback;
        glm::vec2          _pressedContentDelta{2, 2};

        State              _state = State::kNormal;

        std::string        _normalSprite;
        std::string        _hoveredSprite;
        std::string        _pressedSprite;
        std::string        _iconSprite;
        std::string        _textLabel;

        glm::vec2          _iconPosition{0, 0};
        glm::vec2          _iconSize{0, 0};

        glm::vec2          _textPosition{0, 0};
        glm::vec2          _textSize{0, 0};

        friend class Dropdown;
    };
}
