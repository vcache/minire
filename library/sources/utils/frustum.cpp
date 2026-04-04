#include <utils/frustum.hpp>

#include <glm/glm.hpp>  // for normalize

#include <cassert>
#include <cmath>
#include <limits>

namespace minire::utils
{
    namespace
    {
        bool almostZero(float const value)
        {
            return std::abs(value) < std::numeric_limits<float>::epsilon() * 10.0f;
        }
    }

    glm::vec3 ViewFrustum::direction() const
    {
        glm::vec3 const near = almostZero(_nearPlane.w) ? glm::vec3(_nearPlane)
                                                        : _nearPlane / _nearPlane.w;

        glm::vec3 const far  = almostZero(_farPlane.w)  ? glm::vec3(_farPlane)
                                                        : _farPlane / _farPlane.w;
        return glm::normalize(far - near);
    }

    // Returns a point where view direction hits Y-plane
    glm::vec3 ViewFrustum::directionYHitPoint() const
    {
        glm::vec3 const near = almostZero(_nearPlane.w) ? glm::vec3(_nearPlane)
                                                        : _nearPlane / _nearPlane.w;

        glm::vec3 const far  = almostZero(_farPlane.w)  ? glm::vec3(_farPlane)
                                                        : _farPlane / _farPlane.w;

        glm::vec3 const dir  = glm::normalize(far - near);

        if (almostZero(dir.y))
            return near;

        float const t = -near.y / dir.y;
        return glm::vec3(near) + dir * t;
    }

    glm::vec3 ViewFrustum::center() const
    {
        glm::vec3 result = glm::vec3(0);
        for (glm::vec3 const & v : _vertices)
        {
            result += v;
        }
        assert(_vertices.size() != 0);
        result /= static_cast<float>(_vertices.size());
        return result;
    }
}
