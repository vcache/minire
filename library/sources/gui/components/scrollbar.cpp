#include <minire/gui/components/scrollbar.hpp>

#include <minire/errors.hpp>
#include <minire/gui/layout.hpp>
#include <minire/logging.hpp>

#include <cassert>

namespace minire::gui::components
{
    class Scrollbar::CustomLayout
        : public LinearLayout
    {
    public:
        explicit CustomLayout(Scrollbar const & scrollbar)
            : _scrollbar(scrollbar)
        {}

        Area evaluate(Area const & client,
                      Component const & component) const override
        {
            assert(_scrollbar._increaseButton);
            assert(_scrollbar._decreaseButton);

            bool const vertical = _scrollbar._isVertical;

            if (&component == _scrollbar._increaseButton.get())
            {
                float const size = vertical ? client._width : client._height;
                return Area
                {
                    ._left = vertical ? client._left                        : client._left + client._width - size,
                    ._top = vertical  ? client._top + client._height - size : client._top,
                    ._width = size,
                    ._height = size,
                };
            }
            else if (&component == _scrollbar._decreaseButton.get())
            {
                float const size = vertical ? client._width : client._height;
                return Area
                {
                    ._left = client._left,
                    ._top = client._top,
                    ._width = size,
                    ._height = size,
                };
            }
            else if (&component == _scrollbar._slider.get())
            {
                bool const hasIncBtn = _scrollbar._increaseButton->visible().get();
                bool const hasDecBtn = _scrollbar._decreaseButton->visible().get();

                float const size = vertical ? client._width : client._height;
                float const szFactor = (hasDecBtn ? 1 : 0) + (hasIncBtn ? 1 : 0);
                return Area
                {
                    ._left = vertical   ? client._left                         : client._left + (hasDecBtn ? size : 0),
                    ._top = vertical    ? client._top + (hasDecBtn ? size : 0) : client._top,
                    ._width = vertical  ? size                                 : client._width - szFactor*size,
                    ._height = vertical ? client._height - szFactor*size       : size,
                };
            }
            else
            {
                MINIRE_WARNING("unexpected component (\"{}\") inside Scrollbar's container (\"{}\")",
                               component.id(), _scrollbar.id());
                return client;
            }
        }

    private:
        Scrollbar const & _scrollbar;
    };

    namespace
    {
        float clip(float value, float lower, float upper)
        {
            return std::max(lower, std::min(value, upper));
        }

        float normify(float value)
        {
            return clip(value, 0.0f, 1.0f);
        }
    }

    Scrollbar::Scrollbar(std::string const & id,
                         Theme const & theme,
                         Theme::Style const & style,
                         OverlayController & overlayController,
                         bool isVertical)
        : Image(id, theme, style, overlayController,
                theme.get<minire::models::sprite::MaybeImage>("scrollbar", "bg", style))
        , _step(*this, 0.1f)
        , _minSliderLength(*this, theme.get<float>("scrollbar", "min-slider-length", style))
        , _value(*this, 0.0f)
        , _increaseButton(std::make_shared<Button>("__incBtn__", theme, style, overlayController, false, true)) // TODO: cascade style "scrollbar" <- "button"
        , _decreaseButton(std::make_shared<Button>("__decBtn__", theme, style, overlayController, false, true)) // TODO: cascade style "scrollbar" <- "button"
        , _slider(std::make_shared<Button>("__slider__", theme, style, overlayController, false, true))         // TODO: cascade style "scrollbar" <- "slider-bg"
        , _isVertical(isVertical)
    {
        layout() = std::make_shared<CustomLayout>(*this);
    }

    void Scrollbar::initialize()
    {
        Image::initialize();

        auto sharedThis = shared_from_this();

        // TODO: why create it here things that will be rewritten in revalidate()

        // Increase button
        assert(_increaseButton);
        _increaseButton->setParent(sharedThis);
        _increaseButton->setCallback(std::in_place_type<gui::OnClick>, "__scrollbar__",
            [this](Component const &, gui::OnClick const &)
            { setValue(_value.get() + _step.get()); });
        _increaseButton->icon() = theme().get<minire::models::sprite::MaybeImage>("scrollbar",
            _isVertical ? "i:arrow-down" : "i:arrow-right",
            style());

        // Decrease button
        assert(_decreaseButton);
        _decreaseButton->setParent(sharedThis);
        _decreaseButton->setCallback(std::in_place_type<gui::OnClick>, "__scrollbar__",
            [this](Component const &, gui::OnClick const &)
            { setValue(_value.get() - _step.get()); });
        _decreaseButton->icon() = theme().get<minire::models::sprite::MaybeImage>("scrollbar",
            _isVertical ? "i:arrow-up" : "i:arrow-left",
            style());

        // Slider button
        assert(_slider);
        _slider->setParent(sharedThis);
        _slider->isDraggable() = true;
        _slider->setCallback(std::in_place_type<gui::OnDragBegin>, "__scrollbar__",
            [this](Component const &, gui::OnDragBegin const &)
            {
                _dragInitialOffset = _sliderAreaBoundaries.first +
                    normify(_value.get()) * (_sliderAreaBoundaries.second - _sliderLength);
            });

        _slider->setCallback(std::in_place_type<gui::OnDragMove>, "__scrollbar__",
            [this](Component const &, gui::OnDragMove const & e)
            {
                int const begin = _isVertical ? e._begin._y : e._begin._x;
                int const offset = _isVertical ? e._event._absY : e._event._absX;
                setValueFromPosition(static_cast<float>(
                    _dragInitialOffset + (offset - begin)));
            });

        _slider->setCallback(std::in_place_type<application::OnMouseWheel>,
            "__scrollbar__",
            [this](Component const &, application::OnMouseWheel const & e)
            {
                handle(e);
            });

        // Background interaction
        this->isDraggable() = true;
        this->setCallback(std::in_place_type<gui::OnDragBegin>, "__scrollbar__",
            [this](Component const &, gui::OnDragBegin const & e)
            {
                int const abs = _isVertical ? e._event._y : e._event._x;
                setValueFromPosition(static_cast<float>(abs - _sliderLength/2));
            });

        this->setCallback(std::in_place_type<gui::OnDragMove>, "__scrollbar__",
            [this](Component const &, gui::OnDragMove const & e)
            {
                int const abs = _isVertical ? e._event._absY : e._event._absX;
                setValueFromPosition(static_cast<float>(abs - _sliderLength/2));
            });
    }


    void Scrollbar::setValueFromPosition(float abs)
    {
        float const lower = _sliderAreaBoundaries.first;
        float const upper = _sliderAreaBoundaries.first + _sliderAreaBoundaries.second - _sliderLength;
        abs = clip(abs, lower, upper);
        float const previous = _value.get();
        _value = normify((abs - lower) / (upper - lower));
        if (_value.isInvalidated())
        {
            handle(scrollbar::OnValueChanged(previous, _value.get()));
        }
    }

    void Scrollbar::setValue(float value)
    {
        float const previous = _value.get();
        _value = normify(value);
        if (_value.isInvalidated())
        {
            handle(scrollbar::OnValueChanged(previous, _value.get()));
        }
    }

    size_t Scrollbar::revalidateContent(size_t zOffset,
                                        bool const effectiveVisible,
                                        Area const & contentArea,
                                        Area const & clippingWindow)
    {
        zOffset = Image::revalidateContent(
            zOffset, effectiveVisible, contentArea, clippingWindow);

        // calculate slider's max offset
        bool const hasIncBtn = _increaseButton->visible().get();
        bool const hasDecBtn = _decreaseButton->visible().get();
        float const buttonSize = _isVertical ? contentArea._width : contentArea._height;
        float const btnDeltas =  (hasIncBtn ? buttonSize : 0) + (hasDecBtn ? buttonSize : 0);
        Boundaries const sliderAreaBoundaries
        {
            (_isVertical ? contentArea._top    : contentArea._left)  + (hasDecBtn ? buttonSize : 0),
            (_isVertical ? contentArea._height : contentArea._width) - btnDeltas,
        };

        // revalidate slider size and position
        assert(_slider);
        if (_step.isInvalidated() ||
            _value.isInvalidated() ||
            _minSliderLength.isInvalidated() ||
            sliderAreaBoundaries.second != _sliderAreaBoundaries.second)
        {
            float const sliderAreaLength = sliderAreaBoundaries.second;

            _sliderLength = std::max(_minSliderLength.get(),
                                     sliderAreaLength * _step.get());

            float const sliderOffset = normify(_value.get()) * (sliderAreaLength - _sliderLength);

            Arranger hArranger(position::Begin{}, dimension::Fill{});
            Arranger vArranger(position::Constant{sliderOffset},
                               dimension::Constant{_sliderLength});

            _slider->horizontal() = _isVertical ? hArranger : vArranger;
            _slider->vertical() = _isVertical ? vArranger : hArranger;
        }

        // finish

        _sliderAreaBoundaries = sliderAreaBoundaries;

        _value.revalidate();
        _step.revalidate();
        _minSliderLength.revalidate();

        return zOffset;
    }

    Button const & Scrollbar::increaseButton() const
    {
        assert(_increaseButton);
        return *_increaseButton;
    }

    Button const & Scrollbar::decreaseButton() const
    {
        assert(_decreaseButton);
        return *_decreaseButton;
    }

    Button & Scrollbar::increaseButton()
    {
        assert(_increaseButton);
        return *_increaseButton;
    }

    Button & Scrollbar::decreaseButton()
    {
        assert(_decreaseButton);
        return *_decreaseButton;
    }

    void Scrollbar::handle(application::OnMouseWheel const & e)
    {
        setValue(_value.get() + _step.get() * static_cast<float>(-e._dy));
        Component::handle(e);
    }
}
