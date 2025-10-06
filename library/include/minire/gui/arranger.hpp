#pragma once

#include <cassert>
#include <cmath>
#include <optional>
#include <utility>
#include <variant>

namespace minire::gui
{
    namespace position
    {
        struct Constant
        {
            float _position;

            bool operator==(Constant const &) const = default;
        };

        struct Less
        {
            bool operator==(Less const &) const = default;
        };

        struct Center
        {
            bool operator==(Center const &) const = default;
        };

        struct More
        {
            bool operator==(More const &) const = default;
        };
    }

    using Position = std::variant<position::Constant,
                                  position::Less,
                                  position::Center,
                                  position::More>;

    namespace dimension
    {
        struct Constant
        {
            float _dimension;

            bool operator==(Constant const &) const = default;
        };

        struct Fraction
        {
            float _fraction;

            bool operator==(Fraction const &) const = default;
        };

        struct Fill
        {
            bool operator==(Fill const &) const = default;
        };

        struct Content
        {
            bool operator==(Content const &) const = default;
        };
    }

    using Dimension = std::variant<dimension::Constant,
                                   dimension::Fraction,
                                   dimension::Fill,
                                   dimension::Content>;

    class Arranger
    {
    public:
        explicit Arranger(Position const & position = position::Center{},
                          Dimension const & dimension = dimension::Fill{},
                          float marginMin = 0,
                          float marginMax = 0)
            : _position(position)
            , _dimension(dimension)
            , _marginMin(marginMin)
            , _marginMax(marginMax)
        {}

        // (position, dimension)
        std::pair<float, float> operator()(float clientPosition,
                                           float clientDimension,
                                           std::optional<float> contentDimension) const;

        Position const & position() const { return _position; }
        Dimension const & dimension() const { return _dimension; }
        float marginMin() const { return _marginMin; }
        float marginMax() const { return _marginMax; }

        void setPosition(Position const & position) { _position = position; }
        void setDimension(Dimension const & dimension) { _dimension = dimension; }
        void setMarginMin(float marginMin) { _marginMin = marginMin; }
        void setMarginMax(float marginMax) { _marginMax = marginMax; }

        bool operator==(Arranger const &) const = default;

    private:
        Position  _position;
        Dimension _dimension;
        float     _marginMin;
        float     _marginMax;
    };

    struct Arrangers
    {
        Arranger _horizontal;
        Arranger _vertical;

        bool operator==(Arrangers const &) const = default;

    public:
        static Arrangers const & fill();
        static Arrangers const & center();
    };
}
