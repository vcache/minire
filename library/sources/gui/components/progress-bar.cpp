#include <minire/gui/components/progress-bar.hpp>

#include <minire/errors.hpp>

#include <glm/common.hpp>

namespace minire::gui::components
{
    ProgressBar::ProgressBar(std::string const & id,
                             Theme const & theme,
                             OverlayController & overlayController,
                             Direction const direction)
        : Component(id, theme, overlayController)
        , _background(*this, theme.progressBar().makeBackground())
        , _slider(*this, theme.progressBar().makeSlider())
        , _value(*this, 0.0f)
        , _direction(*this, direction)
        , _sliderPadding(*this, theme.progressBar().constants()._sliderPadding)
    {}

    size_t ProgressBar::revalidateContent(size_t zOffset,
                                          bool const effectiveVisible,
                                          Area const & contentArea,
                                          Area const & clippingWindow)
    {
        // revalidate background
        if (auto background = _background.get(); background)
        {
            if (_background.isInvalidated())
            {
                background->setContentInvalidator(shared_from_this());
            }

            background->setVisible(effectiveVisible);
            if (effectiveVisible)
            {
                background->setContentArea(contentArea);
                background->setClippingWindow(clippingWindow);
            }

            zOffset = background->onZOrderChanged(zOffset);
        }

        // revalidate a slider
        if (auto slider = _slider.get(); slider)
        {
            if (_slider.isInvalidated())
            {
                slider->setContentInvalidator(shared_from_this());
            }

            slider->setVisible(effectiveVisible);
            if (effectiveVisible)
            {
                utils::Rect const & sliderPadding = _sliderPadding.get();
                Area const realContentArea
                {
                    ._left = contentArea._left + sliderPadding._left,
                    ._top = contentArea._top + sliderPadding._top,
                    ._width = contentArea._width - sliderPadding._left - sliderPadding._right,
                    ._height = contentArea._height - sliderPadding._top - sliderPadding._bottom,
                };

                slider->setContentArea(evalSliderArea(realContentArea));
                slider->setClippingWindow(realContentArea);
            }

            zOffset = slider->onZOrderChanged(zOffset);
        }

        // finish
        _background.revalidate();
        _slider.revalidate();
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