#include <minire/gui/components/scrollbar.hpp>

#include <minire/errors.hpp>
#include <minire/gui/layout.hpp>
#include <minire/logging.hpp>

namespace minire::gui::components
{
    class Scrollbar::CustomLayout
        : public Layout
    {
    public:
        explicit CustomLayout(Scrollbar const & scrollbar)
            : _scrollbar(scrollbar)
        {}

        Area evaluate(Area const & client,
                      Component const & component) const override
        {
            bool const vertical = _scrollbar._vertical;
            bool const hasIncBtn = _scrollbar._increase.operator bool();
            bool const hasDecBtn = _scrollbar._decrease.operator bool();

            if (&component == _scrollbar._increase.get())
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
            else if (&component == _scrollbar._decrease.get())
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
            else if (&component == _scrollbar._background.get())
            {
                return client;
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

    Scrollbar::Scrollbar(GuiController & controller,
                         std::string const & id,
                         std::shared_ptr<Container> const & parent)
        : Container(controller, id, parent,
                    std::make_shared<Scrollbar::CustomLayout>(*this))
    {}

    void Scrollbar::onContentAreaChanged()
    {
        Container::onContentAreaChanged();

        Area const & area = contentArea();
        float const buttonSize = _vertical ? area._width : area._height;
        float const btnDeltas =  (_increase ? buttonSize : 0) + (_decrease ? buttonSize : 0);

        float const newSliderAmplitude = _vertical ? area._height - btnDeltas
                                                   : area._width - btnDeltas;

        // NOTE: it _might_ cause an infinite recrusion
        setSliderAmplitude(newSliderAmplitude);
    }

    void Scrollbar::init(bool vertical,
                         Background const & background,
                         Button::Sptr const & increase,
                         Button::Sptr const & decrease,
                         Button::Sptr const & slider,
                         Arrangers arrangers)
    {
        _vertical = vertical;

        _background = emplace<Button>("__bg__", Button::Background{background._texture,
                                                                   background._patch,
                                                                   background._patch,
                                                                   background._patch},
                                      std::nullopt, std::nullopt, Arrangers::fill());
        _background->setIsDragable(true);
        _background->setDragMoveCallback(
            [this](Component &, events::application::OnMouseMove const & e)
            {
                Area const & area = _slider->clientArea();

                float const abs = _vertical ? e._absY : e._absX;
                float const rel = _vertical ? e._relY : e._relX;
                float const activeAmplitude = _sliderAmplitude - sliderSize();
                float const lowerBound = _vertical ? area._top : area._left;
                float const upperBound = lowerBound + _sliderAmplitude;
                float const newSliderOffset =
                    abs < lowerBound ? 0 : (abs >= upperBound ? _sliderAmplitude - sliderSize()
                                                              : _sliderOffset + rel);
                if (activeAmplitude > 0)
                    setValue(newSliderOffset / activeAmplitude);
            });

        _background->setDragBeginCallback(
            [this](Component const &, events::application::OnMouseDown const & e)
            {
                if (float const activeAmplitude = _sliderAmplitude - sliderSize();
                    activeAmplitude > 0)
                {
                    Area const & area = _slider->clientArea();
                    float const abs = _vertical ? e._y : e._x;
                    float const lowerBound = _vertical ? area._top : area._left;
                    float const upperBound = lowerBound + activeAmplitude;
                    float const newSliderOffset = std::min(abs - lowerBound - sliderSize() * .5f, upperBound);
                    setValue(newSliderOffset / activeAmplitude);
                }
            });

        _increase = increase;
        _decrease = decrease;
        _slider = slider;

        if (increase)
        {
            emplace(increase);
            increase->setArrangers(Arrangers::fill());
            MINIRE_INVARIANT(!increase->hasClickCallback(),
                             "increase button cannot have a user's click callback");
            increase->setClickCallback([this](Button const &){ setValue(_currentValue + _step); });
        }

        if (decrease)
        {
            emplace(decrease);
            decrease->setArrangers(Arrangers::fill());
            MINIRE_INVARIANT(!decrease->hasClickCallback(),
                             "decrease button cannot have a user's click callback");
            decrease->setClickCallback([this](Button const &){ setValue(_currentValue - _step); });
        }

        if (slider)
        {
            emplace(slider);

            slider->setIsDragable(true);
            slider->setDragMoveCallback(
                [this](Component &, events::application::OnMouseMove const & e)
                {
                    Area const & area = _slider->clientArea();

                    float const abs = _vertical ? e._absY : e._absX;
                    float const rel = _vertical ? e._relY : e._relX;
                    float const activeAmplitude = _sliderAmplitude - sliderSize();
                    float const lowerBound = _vertical ? area._top : area._left;
                    float const upperBound = lowerBound + _sliderAmplitude;
                    float const newSliderOffset = (
                        abs < lowerBound ? 0 : (abs >= upperBound ? _sliderAmplitude - sliderSize()
                                                                  : _sliderOffset + rel));
                    if (activeAmplitude > 0)
                        setValue(newSliderOffset / activeAmplitude);
                });
            updateArrangers();
        }

        setArrangers(arrangers);
    }

    void Scrollbar::setValue(float newValue)
    {
        float const newSliderOffset = newValue * (_sliderAmplitude - sliderSize());

        if (newValue == _currentValue &&
            newSliderOffset == _sliderOffset)
        {
            return;
        }

        float const previousValue = _currentValue;
        _currentValue = std::min(std::max(0.0f, newValue), 1.0f);

        if (_valueChangedCallback &&
            previousValue != _currentValue)
        {
            _valueChangedCallback(*this, previousValue, _currentValue);
        }

        setSliderOfffset(newSliderOffset);
    }

    void Scrollbar::setStep(float newStep)
    {
        if (_step == newStep)
            return;
        _step = std::max(0.0f, newStep);
        setValue(_currentValue);
    }

    void Scrollbar::setMinSliderSize(float newMinSliderSize)
    {
        if (_minSliderSize == newMinSliderSize)
            return;

        _minSliderSize = newMinSliderSize;
        setValue(_currentValue);
    }

    void Scrollbar::updateArrangers()
    {
        if (_slider)
        {
            Arranger hArranger(position::Less{}, dimension::Fill{});
            Arranger vArranger(position::Constant{_sliderOffset},
                               dimension::Constant{sliderSize()});

            _slider->setArrangers(Arrangers
                {
                    ._horizontal = _vertical ? hArranger : vArranger,
                    ._vertical   = _vertical ? vArranger : hArranger,
                });
        }
    }

    void Scrollbar::setSliderOfffset(float newSliderOffset)
    {
        newSliderOffset = std::max(0.0f, std::min(newSliderOffset, _sliderAmplitude - sliderSize()));

        if (newSliderOffset == _sliderOffset)
            return;

        _sliderOffset = newSliderOffset;
        updateArrangers();
    }

    void Scrollbar::setSliderAmplitude(float const newSliderAmplitude)
    {
        if (newSliderAmplitude == _sliderAmplitude ||
            newSliderAmplitude <= 0)
        {
            return;
        }

        _sliderAmplitude = newSliderAmplitude;
        setValue(_currentValue);
        updateArrangers();
    }

    float Scrollbar::sliderSize() const
    {
        return std::max(_minSliderSize, _sliderAmplitude * _step);
    }
}