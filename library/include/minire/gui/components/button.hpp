#pragma once

#include <minire/errors.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/components/text.hpp>
#include <minire/gui/models/checkable.hpp>
#include <minire/label.hpp>
#include <minire/models/sprite.hpp>
#include <minire/sprite.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

#include <memory>

namespace minire::gui::components
{
    // TODO: make hasText and hasIcon mutable
    class Button final
        : public Component
        , public models::Checkable
    {
    public:
        Button(std::string const & id,
               Theme const & theme,
               Theme::Style const & style,
               OverlayController &,
               bool const hasText,
               bool const hasIcon);

        using Sptr = std::shared_ptr<Button>;
        using Wptr = std::weak_ptr<Button>;

        using CommonCallbacks::handle;
        using CommonCallbacks::setCallback;
        using CommonCallbacks::eraseCallback;
        using Checkable::handle;
        using Checkable::setCallback;
        using Checkable::eraseCallback;

        // Background

        Property<minire::models::sprite::MaybeImage> const & bgNormal() const { assert(_bgNormal); return _bgNormal->image(); }
        Property<minire::models::sprite::MaybeImage> & bgNormal() { assert(_bgNormal); return _bgNormal->image(); }

        Property<minire::models::sprite::MaybeImage> const & bgHovered() const { assert(_bgHovered); return _bgHovered->image(); };
        Property<minire::models::sprite::MaybeImage> & bgHovered() { assert(_bgHovered);  return _bgHovered->image(); };

        Property<minire::models::sprite::MaybeImage> const & bgPressed() const { assert(_bgPressed); return _bgPressed->image(); }
        Property<minire::models::sprite::MaybeImage> & bgPressed() {assert(_bgPressed); return _bgPressed->image(); }

        // Caption

        Property<text::FormattedString> const & text() const
        {
            MINIRE_INVARIANT(_hasText, "a button doesn't have a text");
            assert(_text);
            return _text->text();
        }
        Property<text::FormattedString> & text()
        {
            MINIRE_INVARIANT(_hasText, "a button doesn't have a text");
            assert(_text);
            return _text->text();
        }

        Property<content::Id> const & fontFace() const
        {
            MINIRE_INVARIANT(_hasText, "a button doesn't have a text");
            assert(_text);
            return _text->fontFace();
        }
        Property<content::Id> & fontFace()
        {
            MINIRE_INVARIANT(_hasText, "a button doesn't have a text");
            assert(_text);
            return _text->fontFace();
        }

        // Icon

        Property<minire::models::sprite::MaybeImage> const & icon() const
        {
            MINIRE_INVARIANT(_hasIcon, "a button doesn't have an icon");
            assert(_icon);
            return _icon->image();
        }
        Property<minire::models::sprite::MaybeImage> & icon()
        {
            MINIRE_INVARIANT(_hasIcon, "a button doesn't have an icon");
            assert(_icon);
            return _icon->image();
        }

        Property<Theme::Location> const & iconLocation() const
        {
            MINIRE_INVARIANT(_hasIcon, "a button doesn't have an icon");
            return _iconLocation;
        }
        Property<Theme::Location> & iconLocation()
        {
            MINIRE_INVARIANT(_hasIcon, "a button doesn't have an icon");
            return _iconLocation;
        }

        Property<float> const & iconSpacing() const
        {
            MINIRE_INVARIANT(_hasIcon, "a button doesn't have an icon");
            return _iconSpacing;
        }
        Property<float> & iconSpacing()
        {
            MINIRE_INVARIANT(_hasIcon, "a button doesn't have an icon");
            return _iconSpacing;
        }

        // Miscellaneous

        Property<glm::vec2> const & pressOffset() const { return _pressOffset; }
        Property<glm::vec2> & pressOffset() { return _pressOffset; }

        Property<utils::Rect> const & contentPadding() const { return _contentPadding; }
        Property<utils::Rect> & contentPadding() { return _contentPadding; }

    protected:
        void initialize() override;

        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

        std::optional<glm::vec2> measureContent() const override;

        void handle(models::checkable::OnCheckedChanged const &) override;
        void handle(application::OnMouseDown const & e) override;
        void handle(gui::OnDragEnd const &) override;
        void handle(gui::OnMouseEnter const &) override;
        void handle(gui::OnMouseLeave const &) override;
        void handle(gui::OnClick const &) override;

    private:
        enum State
        {
            kNormal, kHovered, kPressed,
        };

        void setState(State state);

        void rebuildLayout();
        void updateArrangers();
        void updateBackground();

    private:
        components::Image::Sptr   _bgNormal;
        components::Image::Sptr   _bgHovered;
        components::Image::Sptr   _bgPressed;
        // TODO: focused, hovered while pressed, and etc

        Component::Sptr           _content; // container for text+icon
        components::Text::Sptr    _text;
        components::Image::Sptr   _icon;

        Property<Theme::Location> _iconLocation;
        Property<float>           _iconSpacing;
        Property<glm::vec2>       _pressOffset;
        Property<utils::Rect>     _contentPadding;

        State                     _state = State::kNormal;
        glm::vec2                 _textPosition{0, 0};
        glm::vec2                 _iconPosition{0, 0};

        bool const                _hasText;
        bool const                _hasIcon;
    };
}
