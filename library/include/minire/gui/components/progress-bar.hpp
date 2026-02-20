#pragma once

#include <minire/gui/component.hpp>
#include <minire/gui/content-view.hpp>
#include <minire/utils/rect.hpp>

namespace minire::gui::components
{
    class ProgressBar final
        : public Component
    {
    public:
        using Sptr = std::shared_ptr<ProgressBar>;
        using Wptr = std::weak_ptr<ProgressBar>;

        enum class Direction
        {
            kLeftToRight,
            kRightToLeft,
            kTopToBottom,
            kBottomToTop,
        };

        ProgressBar(std::string const & id,
                    Theme const & theme,
                    OverlayController &,
                    Direction const);

        Property<ImageView::Sptr> const & background() const { return _background; }
        Property<ImageView::Sptr> & background() { return _background; }

        Property<ImageView::Sptr> const & slider() const { return _slider; }
        Property<ImageView::Sptr> & slider() { return _slider; }

        Property<float> const & value() const { return _value; }
        Property<float> & value() { return _value; }

        Property<Direction> const & direction() const { return _direction; }
        Property<Direction> & direction() { return _direction; }

        Property<utils::Rect> const & sliderPadding() const { return _sliderPadding; }
        Property<utils::Rect> & sliderPadding() { return _sliderPadding; }

    protected:
        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

        Area evalSliderArea(Area const & contentArea) const;

    private:
        Property<ImageView::Sptr> _background;
        Property<ImageView::Sptr> _slider;
        Property<float>           _value;
        Property<Direction>       _direction;
        Property<utils::Rect>     _sliderPadding;
    };
}