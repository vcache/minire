#pragma once

#include <type_traits>

namespace minire::utils
{
    template<typename>
    struct kAlwaysFalse : std::false_type {};
}
