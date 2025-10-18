#pragma once

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

        struct Fraction
        {
            float _fraction;

            bool operator==(Fraction const &) const = default;
        };

        struct Begin
        {
            bool operator==(Begin const &) const = default;
        };

        struct Center
        {
            bool operator==(Center const &) const = default;
        };

        struct End
        {
            bool operator==(End const &) const = default;
        };
    }

    using Position = std::variant<position::Constant,
                                  position::Fraction,
                                  position::Begin,
                                  position::Center,
                                  position::End>;

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

    struct Arranger
    {
        Position  _position = position::Center{};
        Dimension _dimension = dimension::Fill{};
        float     _marginMin = 0;
        float     _marginMax = 0;

        // (position, dimension)
        std::pair<float, float> operator()(float clientPosition,
                                           float clientDimension,
                                           std::optional<float> contentDimension) const;

        bool operator==(Arranger const &) const = default;

        static Arranger const & fill();
        static Arranger const & center();
    };
}
