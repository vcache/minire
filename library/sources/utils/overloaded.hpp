#pragma once

namespace minire::utils
{
    template<class... Ts>
    struct Overloaded : Ts...
    {
        using Ts::operator()...;
    };
}
