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

        dimension += (_marginMax + _marginMin);

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

            [clientDimension, dimension]
            (position::Center const &)
            {
                return (clientDimension - dimension) / 2.0f;
            },

            [this, clientDimension, dimension]
            (position::More const &)
            {
                return (clientDimension - dimension);
            }
        }, _position);

        return std::make_pair(std::floor(position),
                              std::floor(dimension));
    }
}
