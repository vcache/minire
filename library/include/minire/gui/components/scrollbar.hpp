#pragma once

#include <minire/gui/components/button.hpp>
#include <minire/gui/components/container.hpp>
#include <minire/gui/components/image.hpp>

#include <functional>
#include <memory>
#include <string>

namespace minire::gui::components
{
    class Scrollbar final
        : public Container
    {
    public:
        using Sptr = std::shared_ptr<Scrollbar>;
        using Wptr = std::weak_ptr<Scrollbar>;

        Scrollbar(GuiController & controller,
                  std::string const & id,
                  std::shared_ptr<Container> const & parent);

        struct Background
        {
            content::Id      _texture;
            utils::NinePatch _patch;
        };

        // NOTE: It must be called right after a ctor.
        //       This is workaround for shared_from_this() from a ctor
        //       problem (cannot call this->emplace() from a ctor).
        // TODO: fix it!!
        void init(bool vertical,
                  Background const & background,
                  Button::Sptr const & increase,
                  Button::Sptr const & decrease,
                  Button::Sptr const & slider,
                  Arrangers arrangers = Arrangers::fill());

    public:
        float value() const { return _currentValue; }
        void setValue(float);

        float step() const { return _step; }
        void setStep(float);

        float minSliderSize() const { return _minSliderSize; }
        void setMinSliderSize(float);

    public:
        using ValueChangedCallback =
            std::function<void(Scrollbar &, float previous, float current)>;

        template<typename Callback>
        void setValueChangedCallback(Callback callback)
        {
            _valueChangedCallback = callback;
        }

        bool hasValueChangedCallback() const { return _valueChangedCallback.operator bool(); }

    protected:
        void onContentAreaChanged() override;

    private:
        void updateArrangers();
        void setSliderOfffset(float);
        void setSliderAmplitude(float const);

        float sliderSize() const;

    private:
        class CustomLayout;

        Button::Sptr         _background;
        Button::Sptr         _increase;
        Button::Sptr         _decrease;
        Button::Sptr         _slider;
        ValueChangedCallback _valueChangedCallback;
        float                _currentValue = 0.0f;
        float                _step = 0.1f;
        float                _minSliderSize = 10.0f;
        float                _sliderOffset = 0.0f;
        float                _sliderAmplitude = 0.0f;
        bool                 _vertical = false;

        friend class Layout;
    };
}