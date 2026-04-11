#include <minire/gui/arranger.hpp>

#include <minire/errors.hpp>
#include <minire/utils/overloaded.hpp>

namespace minire::gui
{
    std::pair<float, float> Arranger::operator()(float clientPosition,
                                                 float clientDimension,
                                                 std::optional<float> contentDimension) const
    {
        // Fixed position + Fill dimension
        if (std::holds_alternative<position::Constant>(_position) &&
            std::holds_alternative<dimension::Fill>(_dimension))
        {
            float const p = _marginMin + std::get<position::Constant>(_position)._position;
            return std::make_pair(p, clientDimension - p - _marginMax);
        }

        if (std::holds_alternative<position::Fraction>(_position) &&
            std::holds_alternative<dimension::Fill>(_dimension))
        {
            float const p = _marginMin +
                std::get<position::Fraction>(_position)._fraction *
                (clientDimension - _marginMin - _marginMax);
            return std::make_pair(p, clientDimension - p - _marginMax);
        }

        // Floating position + Fixed dimension
        float visibleDimension = std::visit(utils::Overloaded
        {
            [this](dimension::Constant const & v)
            {
                return v._dimension;
            },

            [this, clientDimension](dimension::Fraction const & v)
            {
                return clientDimension * v._fraction - _marginMin - _marginMax;
            },

            [this, clientDimension](dimension::Fill const &)
            {
                return clientDimension - _marginMin - _marginMax;
            },

            [this, &contentDimension](dimension::Content const &)
            {
                MINIRE_INVARIANT(contentDimension, "content size is immesuarable");
                return *contentDimension;
            }
        }, _dimension);

        float position = clientPosition + std::visit(utils::Overloaded
        {
            [this](position::Constant const & v)
            {
                return v._position + _marginMin;
            },

            [this, clientDimension]
            (position::Fraction const & v)
            {
                return (clientDimension - _marginMin - _marginMax) * v._fraction + _marginMin;
            },

            [this](position::Begin const &)
            {
                return _marginMin;
            },

            [this, clientDimension, visibleDimension]
            (position::Center const &)
            {
                float const fullDimension = visibleDimension + _marginMin + _marginMax;
                return _marginMin + (clientDimension - fullDimension) / 2.0f;
            },

            [this, clientDimension, visibleDimension]
            (position::End const &)
            {
                return clientDimension - visibleDimension - _marginMax;
            }
        }, _position);

        return std::make_pair(std::floor(position),
                              std::floor(visibleDimension));
    }

    Arranger const & Arranger::fill()
    {
        static const Arranger kResult(position::Begin{}, dimension::Fill{});
        return kResult;
    }

    Arranger const & Arranger::center()
    {
        static const Arranger kResult(position::Center{}, dimension::Content{});
        return kResult;
    }
}
