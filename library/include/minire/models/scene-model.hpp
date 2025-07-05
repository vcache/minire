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
        std::string _id; // TODO [X] useless method? or make it content::Id
        content::Id _mesh;
        Map         _albedo;
        Map         _metallic;
        Map         _roughness;
        Map         _ao;
        Map         _normals;
    };
}
