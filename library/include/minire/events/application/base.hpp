#pragma once

#include <utility>

#   ifndef NDEBUG
#   include <minire/utils/unow.hpp>
#   endif

namespace minire::events::application
{
    struct Base
    {
        // NOTE: Not adding virtual dtor, because it is not supposed to be
        //       used polymorphically.

#       ifndef NDEBUG
        size_t _createTime;

        Base()
            : _createTime(utils::uNow())
        {}
#       endif
    };
}
