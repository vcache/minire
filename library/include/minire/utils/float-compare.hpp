#pragma once

#include <cmath>
#include <limits>
#include <optional>
#include <type_traits>

namespace minire::utils
{
    template<typename T>
    bool isNear(T a, T b)
    {
        static_assert(std::is_floating_point_v<std::decay_t<T>>,
                      "isNear is only applicable to floating-point types");
        return std::fabs(a - b) < std::numeric_limits<T>::epsilon() * T(100);
    }

    template<typename T>
    bool isNear(std::optional<T> const & a,
                std::optional<T> const & b)
    {
        if (a.has_value() != b.has_value()) return false;
        return a.has_value() ? isNear(*a, *b) : true;
    }
}
