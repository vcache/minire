#pragma once

#include <minire/models/scene-queries.hpp>

namespace minire::events::application
{
    struct Base
    {
        models::SceneTraits _sceneTraits;

        explicit Base(models::SceneTraits sceneTraits)
            : _sceneTraits(std::move(sceneTraits))
        {}

        // NOTE: Not adding virtual dtor, because it is not supposed to be
        //       used polymorphically.
    };
}
