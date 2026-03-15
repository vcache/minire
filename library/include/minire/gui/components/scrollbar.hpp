#pragma once

#include <minire/gui/component.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/components/image.hpp>

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

    class Scrollbar
        : public components::Image
        , public Callback<Scrollbar, scrollbar::OnValueChanged>
    {
    public:
        Scrollbar(std::string const & id,
                  Theme const & theme,
                  Theme::Style const & style,
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

        Property<minire::models::sprite::MaybeImage> const & background() const { return Image::image(); }
        Property<minire::models::sprite::MaybeImage> & background() { return Image::image(); }

        Property<float> const & step() const { return _step; }
        Property<float> & step() { return _step; }

        Property<float> const & minSliderLength() const { return _minSliderLength; }
        Property<float> & minSliderLength() { return _minSliderLength; }

        bool isVertical() const { return _isVertical; }

        virtual void handle(application::OnMouseWheel const &) override;

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

        Property<float> _step;
        Property<float> _minSliderLength;
        Property<float> _value;

        Button::Sptr    _increaseButton;
        Button::Sptr    _decreaseButton;
        Button::Sptr    _slider;

        Boundaries      _sliderAreaBoundaries{0, 0};
        float           _sliderLength = 0;
        float           _dragInitialOffset = 0;
        bool const      _isVertical;

        class CustomLayout;
        friend class CustomLayout;
    };

    // Handy shortcuts

    class VerticalScrollbar final
        : public Scrollbar
    {
    public:
        VerticalScrollbar(std::string const & id,
                          Theme const & theme,
                          Theme::Style const & style,
                          OverlayController & overlayController)
            : Scrollbar(id, theme, style, overlayController, true)
        {}

        using Sptr = std::shared_ptr<VerticalScrollbar>;
        using Wptr = std::weak_ptr<VerticalScrollbar>;
    };

    class HorizontalScrollbar final
        : public Scrollbar
    {
    public:
        HorizontalScrollbar(std::string const & id,
                            Theme const & theme,
                            Theme::Style const & style,
                            OverlayController & overlayController)
            : Scrollbar(id, theme, style, overlayController, false)
        {}

        using Sptr = std::shared_ptr<HorizontalScrollbar>;
        using Wptr = std::weak_ptr<HorizontalScrollbar>;
    };
}
