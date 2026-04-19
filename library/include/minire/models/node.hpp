#pragma once

#include <minire/models/outline.hpp>
#include <minire/models/transform.hpp>

namespace minire::models
{
    struct Node
    {
        models::Transform _origin;
        Outline           _outline = std::monostate();
        bool              _visible = true;
    };
}
