#include <minire/gui/arranger.hpp>

#include <minire/errors.hpp>
#include <utils/overloaded.hpp>

namespace minire::gui
{
    std::pair<float, float> Arranger::operator()(float clientPosition,
                                                 float clientDimension,
                                                 std::optional<float> contentDimension) const
    {
        float dimension = std::visit(utils::Overloaded
        {
            [this](dimension::Constant const & v)
            {
                return v._dimension + _marginMin + _marginMax;
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
                return *contentDimension + _marginMin + _marginMax;
            }
        }, _dimension);

        float position = clientPosition + std::visit(utils::Overloaded
        {
            [this](position::Constant const & v)
            {
                return v._position + _marginMin;
            },

            [this](position::Less const &)
            {
                return _marginMin;
            },

            [this, clientDimension, dimension]
            (position::Center const &)
            {
                return _marginMin + (clientDimension - dimension) / 2.0f;
            },

            [this, clientDimension, dimension]
            (position::More const &)
            {
                return clientDimension - dimension;
            }
        }, _position);

        return std::make_pair(std::floor(position),
                              std::floor(dimension - _marginMin - _marginMax));
    }

    Arrangers const & Arrangers::fill()
    {
        static const Arrangers kResult
        {
            ._horizontal = Arranger(position::Less{}, dimension::Fill{}),
            ._vertical   = Arranger(position::Less{}, dimension::Fill{}),
        };
        return kResult;
    }

    Arrangers const & Arrangers::center()
    {
        static const Arrangers kResult
        {
            ._horizontal = Arranger(position::Center{}, dimension::Content{}),
            ._vertical   = Arranger(position::Center{}, dimension::Content{}),
        };
        return kResult;
    }
}
