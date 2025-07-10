#pragma once

#include <minire/content/id.hpp>

#include <glm/vec3.hpp>

#include <map>
#include <string>
#include <cstdint>
#include <variant>

namespace minire::models
{
    struct SceneModel
    {
        using Map = std::variant<std::monostate,
                                 float,
                                 glm::vec3,
                                 content::Id>;

        struct Mesh
        {
            using Index = std::variant<std::monostate,
                                       size_t>;

            content::Id _source;
            Index       _index; // for example, an index of gLTF's mesh
        };

        Mesh _mesh;
        Map  _albedo;
        Map  _metallic;
        Map  _roughness;
        Map  _ao;
        Map  _normals;
    };
}
