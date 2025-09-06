#pragma once

#include <minire/models/scene-queries.hpp>

namespace minire::events::application
{
    struct Base
    {
        models::SceneTraits _sceneTraits;

        Base(models::SceneTraits sceneTraits)
            : _sceneTraits(std::move(sceneTraits))
        {}
    };
}
