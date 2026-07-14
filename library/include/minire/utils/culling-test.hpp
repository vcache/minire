#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>

namespace minire::utils
{
    struct ViewFrustum; // TODO: maybe make it public?
    class Aabb;

    struct FrustumPlanes
    {
        // for fast rejection phase (SAT Phase 1)
        glm::vec3                _min;
        glm::vec3                _max;

        // for planes test phase (SAT Phase 2)
        // [Near, Far, Left, Right, Top, Bottom]
        std::array<glm::vec4, 6> _planes;
    };

    FrustumPlanes precalcFrustumPlanes(ViewFrustum const &);

    bool cullingTest(Aabb const & worldAabb,
                     FrustumPlanes const & frustumPlanes);
}