#pragma once

#include <minire/content/path.hpp>
#include <minire/utils/aabb.hpp>

namespace minire::content { class Manager; }

namespace minire::utils
{
    Aabb buildAabb(content::Manager &,
                   content::Path const &);
}
