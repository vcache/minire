#pragma once

#include <minire/content/id.hpp>
#include <minire/models/fps-camera.hpp>
#include <minire/models/model-position.hpp>
#include <minire/models/point-light.hpp>

#include <glm/vec4.hpp>

#include <unordered_set>

namespace minire::events::controller
{
    // Cleaners

    struct SceneReset
    {};

    // Ctors

    struct SceneEmergeModel
    {
        size_t                _id;
        content::Id           _model;
        models::ModelPosition _position;
    };

    struct SceneEmergePointLight
    {
        size_t             _id;
        models::PointLight _light;
    };

    // Dtors

    struct SceneUnmergeModel
    {
        size_t _id;
    };

    struct SceneUnmergePointLight
    {
        size_t _id;
    };

    // Mutators

    struct SceneUpdateFpsCamera
    {
        models::FpsCamera _fps;
    };

    struct SceneUpdateModel
    {
        size_t                _id;
        models::ModelPosition _position;
    };

    struct SceneUpdateLight
    {
        size_t             _id;
        models::PointLight _light;
    };

    struct SceneSetSelectedModels
    {
        std::unordered_set<size_t> _ids;
    };
}
