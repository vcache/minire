#pragma once

#include <limits>

namespace minire::utils
{
    template<typename T = size_t>
    static constexpr T kNoValue = std::numeric_limits<T>::max();
}