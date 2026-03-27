#pragma once

#include <minire/models/transform.hpp>

namespace minire::models
{
    struct Node
    {
        models::Transform _origin;
        bool              _visible;
    };
}
