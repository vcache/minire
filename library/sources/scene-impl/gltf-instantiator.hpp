#pragma once

#include <minire/content/path.hpp>

namespace minire::content { class Manager; }
namespace minire::scene { class Node; }

namespace minire::scene
{
    void instantiateGltf(scene::Node & parent,
                         content::Path const & source,
                         content::Manager &,
                         bool visible);
}
