#include <utils/culling-test.hpp>

#include <utils/frustum.hpp>

#include <minire/utils/aabb.hpp>

#include <array>
#include <algorithm>

namespace minire::utils
{
    SatPlanes precalcSatPlanes(ViewFrustum const & viewFrustum)
    {
        // frustum AABB
        glm::vec3 min = viewFrustum[0];
        glm::vec3 max = viewFrustum[0];
        for (size_t i = 1; i < viewFrustum.size(); ++i)
        {
            min = glm::min(min, viewFrustum[i]);
            max = glm::max(max, viewFrustum[i]);
        }

        // helper to generate a plane from 3 points
        auto makePlane = [&viewFrustum, center = viewFrustum.center()]
            (int i0, int i1, int i2) -> glm::vec4
            {
                glm::vec3 const v0 = viewFrustum[i0];
                glm::vec3 const v1 = viewFrustum[i1];
                glm::vec3 const v2 = viewFrustum[i2];

                glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
                float len = glm::length(normal);

                if (len > 1e-5f)
                {
                    normal /= len;
                }
                else
                {
                    // emergency fallback
                    normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                // ensure normal strictly points outwards (away from center of frustum)
                if (glm::dot(normal, center - v0) > 0.0f)
                {
                    normal = -normal;
                }
                return glm::vec4(normal, -glm::dot(normal, v0));
            };

        // construct the 6 Frustum planes
        return SatPlanes
        {
            ._min = min,
            ._max = max,
            ._planes = std::array<glm::vec4, 6>
            {
                makePlane(0, 1, 2), // Near
                makePlane(4, 5, 6), // Far
                makePlane(0, 4, 5), // Left
                makePlane(3, 7, 6), // Right
                makePlane(1, 5, 6), // Top
                makePlane(0, 4, 7)  // Bottom
            }
        };
    }

    // Separating Axis Theorem (SAT)
    //
    // NOTE: This code is heavy rely on the order of ViewFrustum::_vertices!
    //       Don't change them without fixing this function.
    bool cullingTest(Aabb const & worldAabb,
                     SatPlanes const & satPlanes)
    {
        // fast rejection (SAT Phase 1): test Frustum bounding box against World AABB
        if (satPlanes._max.x < worldAabb.min().x || satPlanes._min.x > worldAabb.max().x) return false;
        if (satPlanes._max.y < worldAabb.min().y || satPlanes._min.y > worldAabb.max().y) return false;
        if (satPlanes._max.z < worldAabb.min().z || satPlanes._min.z > worldAabb.max().z) return false;

        // test World AABB against the 6 Frustum Planes (SAT Phase 2)
        for (size_t i = 0; i < satPlanes._planes.size(); ++i)
        {
            glm::vec3 const n = glm::vec3(satPlanes._planes[i]);
            float const d = satPlanes._planes[i].w;

            // pick the single AABB vertex that is most "inside" the negative plane normal
            glm::vec3 minPt
            (
                (n.x >= 0.0f) ? worldAabb.min().x : worldAabb.max().x,
                (n.y >= 0.0f) ? worldAabb.min().y : worldAabb.max().y,
                (n.z >= 0.0f) ? worldAabb.min().z : worldAabb.max().z
            );

            // if the furthest inner point is strictly outside the plane, the AABB is completely culled.
            if (glm::dot(n, minPt) + d > 0.0f)
            {
                return false;
            }
        }

        return true; 
    }
}