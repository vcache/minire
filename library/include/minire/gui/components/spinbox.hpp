#pragma once

#include <minire/gui/component.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/components/editbox.hpp>
#include <minire/gui/layouts/array.hpp>

namespace minire::gui::components
{
    namespace spinbox
    {
        struct OnValueChanged
        {
            float _previous;
            float _current;
        };
    }

    class SpinBox final
        : public Component
        , public Callback<SpinBox, spinbox::OnValueChanged>
    {
    public:
        SpinBox(std::string const & id,
                Theme const & theme,
                Theme::Style const & style,
                OverlayController &);

        using Sptr = std::shared_ptr<SpinBox>;
        using Wptr = std::weak_ptr<SpinBox>;

        using CommonCallbacks::handle;
        using CommonCallbacks::setCallback;
        using CommonCallbacks::eraseCallback;
        using Callback<SpinBox, spinbox::OnValueChanged>::handle;
        using Callback<SpinBox, spinbox::OnValueChanged>::setCallback;
        using Callback<SpinBox, spinbox::OnValueChanged>::eraseCallback;

        Button const & decreaseButton() const;
        Button const & increaseButton() const;
        Editbox const & editbox() const;

        Button & decreaseButton();
        Button & increaseButton();
        Editbox & editbox();

        Property<float> const & spacing() const { return _spacing; }
        Property<float> & spacing() { return _spacing; }

        Property<float> const & step() const { return _step; }
        Property<float> & step() { return _step; }

        Property<float> const & minimum() const { return _minimum; }
        void setMinimum(float);

        Property<float> const & maximum() const { return _maximum; }
        void setMaximum(float);

        Property<std::string> const & format() const { return _format; }
        Property<std::string> & format() { return _format; }

        float value() const { return _value.get(); }
        void setValue(float);

        void stepDown();
        void stepUp();

    protected:
        void initialize() override;

        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

    private:
        void refreshView();

    private:
        Property<float>       _spacing;
        Property<float>       _value;
        Property<float>       _step;
        Property<float>       _minimum;
        Property<float>       _maximum;
        Property<std::string> _format;

        Button::Sptr          _decreaseButton;
        Button::Sptr          _increaseButton;
        Editbox::Sptr         _editbox;
        layouts::Array::Sptr  _layout;
    };
}
