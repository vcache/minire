#pragma once

#include <minire/gui/component.hpp>
#include <minire/gui/content-view.hpp>
#include <minire/gui/models/checkable.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

namespace minire::gui::components
{
    class Button final
        : public Component
        , public models::Checkable
    {
    public:
        Button(std::string const & id,
               Theme const & theme,
               OverlayController &);

        using Sptr = std::shared_ptr<Button>;
        using Wptr = std::weak_ptr<Button>;

        using CommonCallbacks::handle;
        using CommonCallbacks::setCallback;
        using CommonCallbacks::eraseCallback;
        using Checkable::handle;
        using Checkable::setCallback;
        using Checkable::eraseCallback;

        Property<ImageView::Sptr> const & bgNormal() const { return _bgNormal; }
        Property<ImageView::Sptr> & bgNormal() { return _bgNormal; }

        Property<ImageView::Sptr> const & bgHovered() const { return _bgHovered; };
        Property<ImageView::Sptr> & bgHovered() { return _bgHovered; };

        Property<ImageView::Sptr> const & bgPressed() const { return _bgPressed; }
        Property<ImageView::Sptr> & bgPressed() { return _bgPressed; }

        Property<TextView::Sptr> const & text() const { return _text; }
        Property<TextView::Sptr> & text() { return _text; }

        Property<ImageView::Sptr> const & icon() const { return _icon; }
        Property<ImageView::Sptr> & icon() { return _icon; }

        Property<theme::Location> const & iconLocation() const { return _iconLocation; }
        Property<theme::Location> & iconLocation() { return _iconLocation; }

        Property<float> const & iconSpacing() const { return _iconSpacing; }
        Property<float> & iconSpacing() { return _iconSpacing; }

        Property<glm::vec2> const & pressOffset() const { return _pressOffset; }
        Property<glm::vec2> & pressOffset() { return _pressOffset; }

        std::optional<std::pair<float, float>> measureContent() const override;

    protected:
        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

        void handle(models::checkable::OnCheckedChanged const &) override;
        void handle(minire::events::application::OnMouseDown const & e) override;
        void handle(gui::events::OnDragEnd const &) override;
        void handle(gui::events::OnMouseEnter const &) override;
        void handle(gui::events::OnMouseLeave const &) override;
        void handle(gui::events::OnClick const &) override;

    private:
        enum State
        {
            kNormal, kHovered, kPressed,
        };

        ImageView::Sptr const & activeBackground() const;
        void revalidatePositions(Area const & contentArea,
                                 Area const & clippingWindow);
        void setState(State state);

        template<typename T>
        void actualize(Property<std::shared_ptr<T>> & contentView);

    private:
        Property<ImageView::Sptr> _bgNormal;
        Property<ImageView::Sptr> _bgHovered;
        Property<ImageView::Sptr> _bgPressed;
        // TODO: focused, hovered while pressed, and etc

        Property<TextView::Sptr>  _text;
        Property<ImageView::Sptr> _icon;
        Property<theme::Location> _iconLocation;
        Property<float>           _iconSpacing;

        Property<glm::vec2>       _pressOffset;

        State                     _state = State::kNormal;
        glm::vec2                 _textPosition{0, 0};
        glm::vec2                 _iconPosition{0, 0};
    };
}
