#pragma once

#include <minire/content/path.hpp>
#include <minire/material.hpp>
#include <minire/models/node-pointer.hpp>
#include <minire/models/outline.hpp>
#include <minire/models/scene-path.hpp>

#include <glm/mat4x4.hpp>

#include <optional>
#include <vector>

namespace minire::models
{
    struct Mesh
    {
        struct Skin
        {
            struct Bone
            {
                glm::mat4   _inverseBindMatrix;
                NodePointer _jointNode;
            };

            using Bones = std::vector<Bone>;

            std::optional<NodePointer> _origin;   // a 'skeleton' Node in an glTF-file
            Bones                      _bones;    // Sorted in the order of appearing in the vertex attribute.
                                                  // I.e. _bones[3] corresponds to JOINTS_0.z
        };

        content::Path         _source;

        // TODO: should it be a default (i.e. fallback) material or
        //       an override-material?
        material::Model::Sptr _defaultMaterial = material::Model::Sptr{};
        std::optional<Skin>   _skin = std::nullopt;
        Outline               _outline = std::monostate();
        glm::vec3             _emissiveFactor = glm::vec3(0);
        bool                  _enableOpb = true; // use mesh in Object Pick Buffer (see Scene::fetch*SceneItem)
        bool                  _visible = true;
    };
}
