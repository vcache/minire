#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>

namespace minire::utils
{
    struct ViewFrustum
    {
        // vertices of a frustum in a World coordinates,
        // (x, y, z) divided by w;
        // Semantics of vertices is following:
        // 0: Near plane bottom-left
        // 1: Near plane top-left
        // 2: Near plane top-right
        // 3: Near plane bottom-right
        // 4: Far plane bottom-left
        // 5: Far plane top-left
        // 6: Far plane top-right
        // 7: Far plane bottom-right
        std::array<glm::vec3, 8> _vertices;

        // additionally, unprojected near/far planes,
        // for example to calculate camera direction
        glm::vec4                _nearPlane;
        glm::vec4                _farPlane;

        auto const & operator[](size_t i) const { return _vertices[i]; }
        auto begin() const { return _vertices.begin(); }
        auto end() const { return _vertices.end(); }
        auto size() const { return _vertices.size(); }

        glm::vec3 direction() const;

        // Returns a point where view direction hits Y-plane
        glm::vec3 directionYHitPoint() const;

        glm::vec3 center() const;
    };
}