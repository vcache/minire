#include <minire/utils/geometry.hpp>

#include <utils/viewpoint.hpp>

#include <fmt/format.h>
#include <glm/common.hpp>

#include <cassert>
#include <limits>

namespace minire::utils
{
    namespace
    {
        inline bool intersects(float a0, float a1, float b0, float b1)
        {
            assert(a0 <= a1);
            assert(b0 <= b1);
            return a0 <= b1 && b0 <= a1;
        }
    }

    bool isIntersects(Aabb const & a, Aabb const & b)
    {
        glm::vec3 delta = glm::abs(a.min() - b.min()) * 2.0f;
        glm::vec3 side = a.dims() + b.dims();
        return delta.x < side.x
            && delta.y < side.y
            && delta.z < side.z;
    }

    bool isIntersectsXZ(Aabb const & a, Aabb const & b)
    {
        return intersects(a.min().x, a.max().x, b.min().x, b.max().x)
            && intersects(a.min().z, a.max().z, b.min().z, b.max().z);
    }

    Ray pixelToWorldRay(glm::vec2 const & windowXy,
                        Viewpoint const & viewpoint)
    {
        glm::vec4 const & viewport = viewpoint.window();

        glm::vec3 win(windowXy.x,
                      viewport.w - windowXy.y,
                      0.0f);

        glm::vec3 const v0 = glm::unProject(win,
                                            viewpoint.view(),
                                            viewpoint.projection(),
                                            viewport);
        win.z = 1.0f;
        glm::vec3 const v1 = glm::unProject(win,
                                            viewpoint.view(),
                                            viewpoint.projection(),
                                            viewport);

        return Ray{viewpoint.position(),
                   glm::normalize(v1 - v0)};
    }

    std::optional<glm::vec3> intersectXZ(Ray const & ray)
    {
        static const glm::vec3 kPlaneNormal(0, 1, 0);
        static const glm::vec3 kPlaneOrigin(0, 0, 0);

        float denom = glm::dot(ray._direction, kPlaneNormal);
        if (fabs(denom) >= std::numeric_limits<float>::epsilon() * 100.0f)
        {
            float dist = glm::dot((kPlaneOrigin - ray._origin), kPlaneNormal) / denom;
            glm::vec3 hitPoint = ray._origin + ray._direction * dist;
            return hitPoint;
        }

        return std::nullopt;
    }

    void GroundHit::extend(glm::vec2 const & point)
    {
        if (_hit)
        {
            _min = glm::min(_min, point);
            _max = glm::max(_max, point);
        }
        else
        {
            _min = _max = point;
        }
        _hit = true;
    }

    void GroundHit::extend(std::optional<glm::vec3> const & point)
    {
        if (point) extend(glm::vec2(point->x, point->z));
    }

    GroundHit intersectXZBoundary(Viewpoint const & viewpoint,
                                  glm::vec2 const & lowerBound,
                                  glm::vec2 const & upperBound)
    {
        glm::vec4 const & viewport = viewpoint.window();

        // boundaries (at the window space)
        glm::vec2 const bound00(viewport.x, viewport.y);
        glm::vec2 const bound01(viewport.x, viewport.w);
        glm::vec2 const bound10(viewport.z, viewport.y);
        glm::vec2 const bound11(viewport.z, viewport.w);

        // rays (at the world space)
        Ray const ray00 = pixelToWorldRay(bound00, viewpoint);
        Ray const ray01 = pixelToWorldRay(bound01, viewpoint);
        Ray const ray10 = pixelToWorldRay(bound10, viewpoint);
        Ray const ray11 = pixelToWorldRay(bound11, viewpoint);

        // test hits
        GroundHit result;
        result.extend(intersectXZ(ray00));
        result.extend(intersectXZ(ray01));
        result.extend(intersectXZ(ray10));
        result.extend(intersectXZ(ray11));

        /*{
            static const glm::vec3 kPlaneNormal(0, 1, 0);
            static size_t kCounter = 0;
            if ((++kCounter) % 10 == 0)
            {
                float d00 = glm::dot(ray00._direction, kPlaneNormal);
                float d01 = glm::dot(ray01._direction, kPlaneNormal);
                float d10 = glm::dot(ray10._direction, kPlaneNormal);
                float d11 = glm::dot(ray11._direction, kPlaneNormal);
                LOGD("------------------------------------------------------------------------ "
                     "DENOMS: " << d00 << ", " << d01 << ", " << d10 << ", " << d11);
            }
        }*/


        // clamp result
        if (result)
        {
            result._min = glm::clamp(result._min, lowerBound, upperBound);
            result._max = glm::clamp(result._max, lowerBound, upperBound);
        }

        return result;
    }
}
