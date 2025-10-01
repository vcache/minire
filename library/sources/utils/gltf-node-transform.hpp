#pragma once

#include <minire/models/transform.hpp>

namespace tinygltf { class Node; }

namespace minire::utils
{
    models::Transform getNodeTransform(::tinygltf::Node const &);
}