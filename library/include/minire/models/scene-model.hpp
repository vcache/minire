#pragma once

#include <minire/content/id.hpp>
#include <minire/material.hpp>

#include <cstdint>
#include <limits>

namespace minire::models
{
    struct SceneModel
    {
        static constexpr size_t kNoIndex = std::numeric_limits<size_t>::max();

        content::Id           _source;
        size_t                _meshIndex = kNoIndex;
        material::Model::Sptr _defaultMaterial;
    };
}
