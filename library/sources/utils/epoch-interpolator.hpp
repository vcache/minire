#pragma once

#include <cmath>

namespace minire::utils
{
    class EpochInterpolator
    {
    public:
        double weight() const
        {
            return fpclassify(_epochLength) == FP_NORMAL
                ? std::min<double>(1.0, _accumulated / _epochLength)
                : 0.0;
        }

        size_t epochNumber() const
        {
            return _epochNumber;
        }

    public:
        void accumulate(double const dT)
        {
            _accumulated += dT;
        }

        void newEpoch(size_t epochNumber,
                      double epochLength)
        {
            _accumulated = 0.0;
            _epochLength = epochLength;
            _epochNumber = epochNumber;
        }

    private:
        double _accumulated = 0.0;
        double _epochLength = 1.0;
        size_t _epochNumber = 0;
    };
}
