#pragma once

#include <minire/gui/component.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/utils/rect.hpp>

namespace minire::gui::components
{
    class ProgressBar final
        : public Image
    {
    public:
        using Sptr = std::shared_ptr<ProgressBar>;
        using Wptr = std::weak_ptr<ProgressBar>;

        static constexpr std::string kName = "ProgressBar";

        enum class Direction
        {
            kLeftToRight,
            kRightToLeft,
            kTopToBottom,
            kBottomToTop,
        };

        ProgressBar(std::string const & id,
                    Theme const & theme,
                    Theme::Style const & style,
                    OverlayController &,
                    Direction const = Direction::kLeftToRight);

        Property<minire::models::sprite::MaybeImage> const & background() const { return Image::image(); }
        Property<minire::models::sprite::MaybeImage> & background() { return Image::image(); }

        Property<minire::models::sprite::MaybeImage> const & slider() const { assert(_slider); return _slider->image(); }
        Property<minire::models::sprite::MaybeImage> & slider() { assert(_slider); return _slider->image(); }

        Property<float> const & value() const { return _value; }
        Property<float> & value() { return _value; }

        Property<Direction> const & direction() const { return _direction; }
        Property<Direction> & direction() { return _direction; }

        Property<utils::Rect> const & sliderPadding() const { return _sliderPadding; }
        Property<utils::Rect> & sliderPadding() { return _sliderPadding; }

    protected:
        void initialize() override;
        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

        Area evalSliderArea(Area const & contentArea) const;

    private:
        components::Image::Sptr _slider;
        Property<float>         _value;
        Property<Direction>     _direction;
        Property<utils::Rect>   _sliderPadding;
    };
}
