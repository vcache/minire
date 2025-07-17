#pragma once

#include <minire/content/id.hpp>
#include <minire/material.hpp>

#include <map>
#include <string>
#include <cstdint>
#include <variant>

namespace minire::models
{
    struct SceneModel
    {
        struct Mesh
        {
            using Index = std::variant<std::monostate,
                                       size_t>;

            content::Id _source;
            Index       _index; // for example, an index of gLTF's mesh
        };

        Mesh                  _mesh;
        material::Model::Sptr _material;
    };
}
