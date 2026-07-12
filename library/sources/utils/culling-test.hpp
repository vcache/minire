#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>

namespace minire::utils
{
    class Aabb;
    class ViewFrustum;

    struct SatPlanes
    {
        // for fast rejection phase (SAT 1)
        glm::vec3                _min;
        glm::vec3                _max;

        // for planes test phase (SAT 2)
        // [Near, Far, Left, Right, Top, Bottom]
        std::array<glm::vec4, 6> _planes;
    };

    SatPlanes precalcSatPlanes(ViewFrustum const &);

    bool cullingTest(Aabb const & worldAabb,
                     SatPlanes const & satPlanes);
}