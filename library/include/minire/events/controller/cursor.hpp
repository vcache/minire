#pragma once

#include <minire/models/system-cursor.hpp>

namespace minire::events::controller
{
    struct SetSystemCursor
    {
        models::SystemCursor _systemCursor;
    };
}
