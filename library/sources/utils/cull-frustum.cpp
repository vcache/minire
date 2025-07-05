#include <utils/cull-frustum.hpp>

#include <fmt/format.h>
#include <glm/gtx/string_cast.hpp>

#include <cmath>

namespace minire::utils
{
    namespace
    {
        void normalizePlane(Frustum::Plane & p)
        {
            p /= std::sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
        }

        std::string toEquation(Frustum::Plane const & p)
        {
            return fmt::format("{}x + {}y + {}z + {} = 0",
                               p.x, p.y, p.z, p.w);
        }

        static inline
        bool isNear(float a, float b)
        {
            return std::fabs(a - b) < 0.0001f; // TODO: std::numeric_limits::epsilon
        }

        // TODO: std::optional
        std::pair<bool, glm::vec2> intersectLines(glm::vec3 const & a,
                                                  glm::vec3 const & b)
        {
            /*
                a.x * x + a.y * z + a.z = 0
                b.x * x + b.y * z + b.z = 0

                z = -(a.x/a.y) * x - (a.z/a.y)
                z = -(b.x/b.y) * x - (b.z/b.y)

                A = -(a.x/a.y)
                B = -(a.z/a.y)
                C = -(b.x/b.y)
                D = -(b.z/b.y)

                Ax + B = Cx + D

                (A - C)x = D - B

                x = (D - B) / (A - C)
            */

            if (!isNear(a.y, 0.0f) &&
                !isNear(b.y, 0.0f))
            {
                float const A = -(a.x/a.y);
                float const B = -(a.z/a.y);
                float const C = -(b.x/b.y);
                float const D = -(b.z/b.y);

                if (!isNear(A, C))
                {
                    float const x = (D - B) / (A - C);
                    float const z = A * x + B;
                    // TODO: failing at tiny values :( assert(feq(z, C * x + D));
                    return std::make_pair(true, glm::vec2(x, z));
                }
            }

            return std::make_pair(false, glm::vec2(0));
        }
    }

    void frustumFromMatrix(glm::mat4 const & m,
                           Frustum & result, 
                           bool const normalize)
    {
        for(int i(0); i < 4; ++i)
        {
            auto const & lcol = m[i][3];

            result._left[i]   = lcol + m[i][0];
            result._right[i]  = lcol - m[i][0];
            result._top[i]    = lcol - m[i][1];
            result._bottom[i] = lcol + m[i][1];
            result._near[i]   = lcol + m[i][2];
            result._far[i]    = lcol - m[i][2];
        }

        if (normalize)
        {
            normalizePlane(result._left);
            normalizePlane(result._right);
            normalizePlane(result._top);
            normalizePlane(result._bottom);
            normalizePlane(result._near);
            normalizePlane(result._far);
        }
    }

    FrustumXZIntersection frustumXZIntersect(Frustum const & f,
                                             glm::vec2 const &/*lowerBound*/,
                                             glm::vec2 const &/*upperBound*/)
    {
        FrustumXZIntersection result;

        // XZ plane equation: y = 0

        // ax + cz + d = 0
        glm::vec3 const lines[6] = {
            glm::vec3(f._left.x,   f._left.z,   f._left.w),
            glm::vec3(f._right.x,  f._right.z,  f._right.w),
            glm::vec3(f._top.x,    f._top.z,    f._top.w),
            glm::vec3(f._bottom.x, f._bottom.z, f._bottom.w),
            glm::vec3(f._near.x,   f._near.z,   f._near.w),
            glm::vec3(f._far.x,    f._far.z,    f._far.w),
        };

        for(int i = 0; i < 6; ++i)
        {
            for(int j = i + 1; j < 6; ++j)
            {
                std::pair<bool, glm::vec2> isect = intersectLines(lines[i], lines[j]);
                if (isect.first)
                {
                    /*glm::vec2 limited = glm::clamp(isect.second,
                                                   lowerBound,
                                                   upperBound);*/
                    glm::vec2 limited = isect.second;
                    if (!result._hit)
                    {
                        result._min = result._max = limited;
                        result._hit = true;
                    }
                    else
                    {
                        result._min = glm::min(result._min, limited);
                        result._max = glm::max(result._max, limited);
                    }
                }
            }
        }

        return result;
    }

    std::string toEquations(Frustum const & frustum)
    {
        return
            "\n" + toEquation(frustum._left) +
            "\n" + toEquation(frustum._right) +
            "\n" + toEquation(frustum._top) +
            "\n" + toEquation(frustum._bottom) +
            "\n" + toEquation(frustum._near) +
            "\n" + toEquation(frustum._far);
    }
}
