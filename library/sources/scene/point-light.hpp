#pragma once

#include <minire/models/point-light.hpp>
#include <utils/lerpable.hpp>

#include <vector>
#include <functional> // for std::reference_wrapper

namespace minire::scene
{
    using PointLight = utils::Lerpable<models::PointLight>;

    struct PointLightRef
    {
        using List = std::vector<std::reference_wrapper<models::PointLight const>>;
    };
}
