#pragma once

#include <minire/utils/aabb.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <optional>

namespace minire::utils
{
    class Viewpoint;

    struct Ray
    {
        glm::vec3 _origin = glm::vec3(0, 0, 0);
        glm::vec3 _direction = glm::vec3(0, 0, 0);
    };

    bool isIntersects(Aabb const &, Aabb const &);
    bool isIntersectsXZ(Aabb const &, Aabb const &);

    Ray pixelToWorldRay(glm::vec2 const &,
                        Viewpoint const &);

    std::optional<glm::vec3> intersectXZ(Ray const &);

    struct GroundHit
    {
        glm::vec2 _min;
        glm::vec2 _max;
        bool      _hit = false;

        operator bool() const { return _hit; }

    private:
        void extend(glm::vec2 const & point);
        void extend(std::optional<glm::vec3> const &);

        friend GroundHit intersectXZBoundary(Viewpoint const &,
                                             glm::vec2 const &,
                                             glm::vec2 const &);
    };

    GroundHit intersectXZBoundary(Viewpoint const & viewpoint,
                                  glm::vec2 const & lowerBound,
                                  glm::vec2 const & upperBound);
}
