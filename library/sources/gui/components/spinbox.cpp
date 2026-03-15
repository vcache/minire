#include <minire/gui/components/spinbox.hpp>

#include <minire/gui/overlay-controller.hpp>

#include <glm/common.hpp>

namespace minire::gui::components
{
    namespace
    {
        std::optional<float> parseFloat(std::string const & s)
        {
            try
            {
                size_t pos = 0;
                float result = std::stof(s, &pos);
                if (pos != s.size())
                {
                    return std::nullopt;
                }
                return result;
            }
            catch(...)
            {
                return std::nullopt;
            }
        }
    }

    SpinBox::SpinBox(std::string const & id,
                     Theme const & theme,
                     Theme::Style const & style,
                     OverlayController & overlayController)
        : Component(id, theme, style, overlayController)
        , _spacing(*this, 0.0f)
        , _value(*this, 0.0f)
        , _step(*this, 0.5f)
        , _minimum(*this, -100.0f)
        , _maximum(*this, 100.0f)
        , _format(*this, "{}")
        , _decreaseButton(std::make_shared<Button>("__decBtn__", theme, style, overlayController, false, true))
        , _increaseButton(std::make_shared<Button>("__incBtn__", theme, style, overlayController, false, true))
        , _editbox(std::make_shared<Editbox>("__edit__", theme, style, overlayController))
    {
        refreshView();
    }

    void SpinBox::initialize()
    {
        auto sharedThis = shared_from_this();

        // build buttons
        assert(_decreaseButton);
        _decreaseButton->setParent(sharedThis);
        _decreaseButton->setCallback(std::in_place_type<gui::OnClick>, "__spinbox__",
            [this](Component const &, gui::OnClick const &)
            { stepDown(); });
        _decreaseButton->icon() =
            theme().parameter<minire::models::sprite::MaybeImage>("spinbox", "i:decrease", style());

        assert(_increaseButton);
        _increaseButton->setParent(sharedThis);
        _increaseButton->setCallback(std::in_place_type<gui::OnClick>, "__spinbox__",
            [this](Component const &, gui::OnClick const &)
            { stepUp(); });
        _increaseButton->icon() =
            theme().parameter<minire::models::sprite::MaybeImage>("spinbox", "i:increase", style());

        // build editbox
        assert(_editbox);
        _editbox->setParent(sharedThis);
        _editbox->horizontal() = Arranger(position::Begin{}, dimension::Fill{});
        _editbox->vertical() = Arranger(position::Begin{}, dimension::Fill{});
        _editbox->setCallback(std::in_place_type<application::OnKeyDown>, "__spinbox__",
            [this](Component const &, application::OnKeyDown const & event)
            {
                if (event._key == SDLK_RETURN)
                {
                    if (std::optional<float> newValue = parseFloat(_editbox->toUtf8());
                        newValue)
                    {
                        setValue(*newValue);
                    }
                    else
                    {
                        refreshView();
                    }
                    overlayController().unfocus();
                }
                else if (event._key == SDLK_ESCAPE)
                {
                    refreshView();
                    overlayController().unfocus();
                }
            });

        _editbox->setCallback(std::in_place_type<gui::OnUnfocus>, "__spinbox__",
            [this](Component const &, gui::OnUnfocus const &)
            {
                if (std::optional<float> newValue = parseFloat(_editbox->toUtf8());
                    newValue)
                {
                    setValue(*newValue);
                }
                else
                {
                    refreshView();
                }
            });

        _editbox->setCallback(std::in_place_type<application::OnMouseWheel>, "__spinbox__",
            [this](Component const &, application::OnMouseWheel const & event)
            {
                int dy = event._dy;
                if (dy > 0)
                {
                    for(; dy != 0; --dy) stepUp();
                }
                else if (dy < 0)
                {
                    for(; dy != 0; ++dy) stepDown();
                }
            });

        // build layouts
        _layout = newLayout<layouts::Array>(true,
            layouts::Array::Elements
            {
                layouts::Array::Element{_decreaseButton->id(), dimension::Content{}},
                layouts::Array::Element{std::nullopt, dimension::Constant{_spacing.get()}},
                layouts::Array::Element{_editbox->id(), dimension::Fill{}},
                layouts::Array::Element{std::nullopt, dimension::Constant{_spacing.get()}},
                layouts::Array::Element{_increaseButton->id(), dimension::Content{}},
            });
    }

    size_t SpinBox::revalidateContent(size_t zOffset,
                                      bool const effectiveVisible,
                                      Area const & /* contentArea */,
                                      Area const & /* clippingWindow */)
    {
        assert(_editbox);

        if (effectiveVisible && !_editbox->hasFocus())
        {
            refreshView();
        }

        _spacing.revalidate();
        _value.revalidate();
        _step.revalidate();
        _minimum.revalidate();
        _maximum.revalidate();
        _format.revalidate();

        return zOffset;
    }

    void SpinBox::setMinimum(float minimum)
    {
        _minimum = minimum;
        if (_minimum.isInvalidated())
        {
            setValue(_value.get());
        }
    }

    void SpinBox::setMaximum(float maximum)
    {
        _maximum = maximum;
        if (_maximum.isInvalidated())
        {
            setValue(_value.get());
        }
    }

    void SpinBox::stepDown()
    {
        setValue(_value.get() - _step.get());
    }

    void SpinBox::stepUp()
    {
        setValue(_value.get() + _step.get());
    }

    void SpinBox::setValue(float value)
    {
        float const prevValue = _value.get();
        _value = glm::clamp(value, _minimum.get(), _maximum.get());
        if (_value.isInvalidated())
        {
            refreshView();

            handle(spinbox::OnValueChanged
                {
                    ._previous = prevValue,
                    ._current = _value.get(),
                });
        }
    }

    void SpinBox::refreshView()
    {
        assert(_editbox);
        _editbox->editText(
            [this](Property<text::FormattedString> & text)
            {
                std::string valueStr = fmt::format(fmt::runtime(_format.get()),
                                                   _value.get());
                text = text::FormattedString(valueStr, _editbox->activeFormat());
            });
    }
}
