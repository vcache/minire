#include <minire/gui/components/progress-bar.hpp>

#include <minire/errors.hpp>

#include <glm/common.hpp>

namespace minire::gui::components
{
    ProgressBar::ProgressBar(std::string const & id,
                             Theme const & theme,
                             Theme::Style const & style,
                             OverlayController & overlayController,
                             Direction const direction)
        : Image(id, theme, style, overlayController,
                theme.parameter<models::sprite::MaybeImage>("progress-bar", "bg", style))
        , _slider(std::make_shared<Image>("__slider__", theme, style, overlayController,
                  theme.parameter<models::sprite::MaybeImage>("progress-bar", "slider", style)))
        , _value(*this, 0.0f)
        , _direction(*this, direction)
        , _sliderPadding(*this, theme.parameter<utils::Rect>("progress-bar", "slider-padding", style))
    {}

    void ProgressBar::initialize()
    {
        Image::initialize();

        assert(_slider);
        _slider->setParent(shared_from_this());
    }

    size_t ProgressBar::revalidateContent(size_t zOffset,
                                          bool const effectiveVisible,
                                          Area const & contentArea,
                                          Area const & clippingWindow)
    {
        zOffset = Image::revalidateContent(
            zOffset, effectiveVisible, contentArea, clippingWindow);

        // revalidate a slider
        if (_value.isInvalidated() ||
            _direction.isInvalidated() ||
            _sliderPadding.isInvalidated())
        {
            assert(_slider);

            _slider->visible() = effectiveVisible && _value.get() > 0;
            if (_slider->visible().get())
            {
                utils::Rect const & sliderPadding = _sliderPadding.get();
                Area const realContentArea // relative to the parent
                {
                    ._left = sliderPadding._left,
                    ._top = sliderPadding._top,
                    ._width = contentArea._width - sliderPadding._left - sliderPadding._right,
                    ._height = contentArea._height - sliderPadding._top - sliderPadding._bottom,
                };

                Area const sliderArea = evalSliderArea(realContentArea);
                _slider->horizontal() = Arranger(position::Constant{sliderArea._left},
                                                 dimension::Constant{sliderArea._width});
                _slider->vertical() = Arranger(position::Constant{sliderArea._top},
                                               dimension::Constant{sliderArea._height});
            }
        }

        // finish
        _value.revalidate();
        _direction.revalidate();
        _sliderPadding.revalidate();

        return zOffset;
    }

    Area ProgressBar::evalSliderArea(Area const & contentArea) const
    {
        float const fraction = glm::clamp(_value.get(), 0.0f, 1.0f);

        switch(_direction.get())
        {
            case Direction::kLeftToRight:
                return Area
                {
                    ._left = contentArea._left,
                    ._top = contentArea._top,
                    ._width = contentArea._width * fraction,
                    ._height = contentArea._height,
                };

            case Direction::kRightToLeft:
                return Area
                {
                    ._left = contentArea._left + (1 - fraction) * contentArea._width,
                    ._top = contentArea._top,
                    ._width = contentArea._width * fraction,
                    ._height = contentArea._height,
                };

            case Direction::kTopToBottom:
                return Area
                {
                    ._left = contentArea._left,
                    ._top = contentArea._top,
                    ._width = contentArea._width,
                    ._height = contentArea._height * fraction,
                };

            case Direction::kBottomToTop:
                return Area
                {
                    ._left = contentArea._left,
                    ._top = contentArea._top + (1 - fraction) * contentArea._height,
                    ._width = contentArea._width,
                    ._height = contentArea._height * fraction,
                };
        }

        MINIRE_THROW("Unexpected slider direction: {}", static_cast<int>(_direction.get()));
    }
}