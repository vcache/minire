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
            [](dimension::Constant const & v)                { return v._dimension; },
            [clientDimension](dimension::Fraction const & v) { return clientDimension * v._fraction; },
            [clientDimension](dimension::Fill const &)       { return clientDimension; },
            [&contentDimension](dimension::Content const &)
            {
                MINIRE_INVARIANT(contentDimension, "content size is immesuarable");
                return *contentDimension;
            }
        }, _dimension);

        dimension -= (_marginMax + _marginMin);

        float position = std::visit(utils::Overloaded
        {
            [this](position::Constant const & v)
            {
                return v._position + _marginMin;
            },

            [this, clientPosition](position::Less const &)
            {
                return clientPosition + _marginMin;
            },

            [clientPosition, clientDimension, dimension]
            (position::Center const &)
            {
                return clientPosition + (clientDimension - dimension) / 2.0f;
            },

            [this, clientPosition, clientDimension, dimension]
            (position::More const &)
            {
                return clientPosition + (clientDimension - dimension - 2.0f * _marginMax);
            }
        }, _position);

        return std::make_pair(std::floor(position),
                              std::floor(dimension));
    }
}