#pragma once

#include <minire/content/id.hpp>

#include <string>
#include <cstdint>

namespace minire::models
{
    struct FontFace
    {
        content::Id _regular;
        content::Id _bold;
        content::Id _italic;
        size_t      _glyphWidth;
        size_t      _glyphHeight;
    };
}
