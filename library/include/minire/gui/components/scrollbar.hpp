#pragma once

#include <minire/gui/component.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/content-view.hpp>

namespace minire::gui::components
{
    namespace scrollbar
    {
        struct OnValueChanged
        {
            float _previous;
            float _current;
        };
    }

    class Scrollbar final
        : public Component
        , public Callback<Scrollbar, scrollbar::OnValueChanged>
    {
    public:
        Scrollbar(std::string const & id,
                  Theme const & theme,
                  OverlayController &,
                  bool isVertical);

        using Sptr = std::shared_ptr<Scrollbar>;
        using Wptr = std::weak_ptr<Scrollbar>;

        using CommonCallbacks::handle;
        using CommonCallbacks::setCallback;
        using CommonCallbacks::eraseCallback;
        using Callback<Scrollbar, scrollbar::OnValueChanged>::handle;
        using Callback<Scrollbar, scrollbar::OnValueChanged>::setCallback;
        using Callback<Scrollbar, scrollbar::OnValueChanged>::eraseCallback;

        Button const & increaseButton() const;
        Button const & decreaseButton() const;
        Button const & slider() const;

        Button & increaseButton();
        Button & decreaseButton();
        Button & slider();

        float value() const { return _value.get(); }
        void setValue(float value);

        Property<ImageView::Sptr> const & background() const { return _background; }
        Property<ImageView::Sptr> & background() { return _background; }

        Property<float> const & step() const { return _step; }
        Property<float> & step() { return _step; }

        Property<float> const & minSliderLength() const { return _minSliderLength; }
        Property<float> & minSliderLength() { return _minSliderLength; }

        Property<bool> const & isVertical() const { return _isVertical; }
        Property<bool> & isVertical() { return _isVertical; }

        virtual void handle(minire::events::application::OnMouseWheel const &) override;

    protected:
        void initialize() override;

        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

    private:
        void setValueFromPosition(float abs);

    private:
        using Boundaries = std::pair<float, float>;

        Property<ImageView::Sptr> _background;
        Property<float>           _step;
        Property<float>           _minSliderLength;
        Property<bool>            _isVertical;
        Property<float>           _value;

        Button::Sptr              _increaseButton;
        Button::Sptr              _decreaseButton;
        Button::Sptr              _slider;

        ImageView::Wptr           _defaultIncreaseIcon;
        ImageView::Wptr           _defaultDecreaseIcon;

        Boundaries                _sliderAreaBoundaries{0, 0};
        float                     _sliderLength = 0;
        float                     _dragInitialOffset = 0;

        class CustomLayout;
        friend class CustomLayout;
    };
}
