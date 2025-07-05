#pragma once

#include <minire/content/id.hpp>

#include <string>
#include <cstdint>

namespace minire::models
{
    struct Font // TODO: rename to FontSet or like that
    {
        std::string _id; // TODO: [X] useless member or make it content::Id
        content::Id _regular;
        content::Id _bold;
        content::Id _italic;
        size_t      _glyphWidth;
        size_t      _glyphHeight;
    };
}
