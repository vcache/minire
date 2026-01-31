#pragma once

#include <minire/content/path.hpp>
#include <minire/material.hpp>
#include <minire/models/scene-path.hpp>

#include <glm/mat4x4.hpp>

#include <optional>
#include <vector>

namespace minire::models
{
    struct Mesh
    {
        content::Path         _source;

        // TODO: should it be a default (i.e. fallback) material or
        //       an override-material?
        material::Model::Sptr _defaultMaterial;
    };

    struct MeshSkin
    {
        struct Bone
        {
            glm::mat4 _inverseBindMatrix;
            ScenePath _jointNode;
        };

        using Bones = std::vector<Bone>;

        std::optional<ScenePath> _origin;   // a 'skeleton' Node in an glTF-file
        Bones                    _bones;    // Sorted in the order of appearing in the vertex attribute.
                                            // I.e. _bones[3] corresponds to JOINTS_0.z
    };
}