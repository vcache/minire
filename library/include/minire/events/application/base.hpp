#pragma once

#include <utility>

namespace minire::events::application
{
    struct Base
    {
        // NOTE: Not adding virtual dtor, because it is not supposed to be
        //       used polymorphically.
    };
}
