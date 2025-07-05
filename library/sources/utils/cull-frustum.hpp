#pragma once

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <string>

namespace minire::utils
{
    // NOTE: viewing frustum actually is an axis-aligned box
    struct Frustum
    {
        using Plane = glm::vec4; // a, b, c, d

        Plane _left;
        Plane _right;
        Plane _top;
        Plane _bottom;
        Plane _near;
        Plane _far;
    };

    void frustumFromMatrix(glm::mat4 const & m,
                           Frustum &,
                           bool const normalize = true);

    // https://www.geogebra.org/3d
    std::string toEquations(Frustum const &);

    struct FrustumXZIntersection
    {
        glm::vec2 _min;
        glm::vec2 _max;
        bool      _hit = false;
    };

    FrustumXZIntersection frustumXZIntersect(Frustum const &,
                                             glm::vec2 const & lowerBound,
                                             glm::vec2 const & upperBound);
}
