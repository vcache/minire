#pragma once

#include <boost/core/demangle.hpp>

namespace minire::utils
{
    template<typename T>
    auto demangle()
    {
        return boost::core::demangle(
            typeid(std::decay_t<T>).name()
        );
    }
}
