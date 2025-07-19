#pragma once

#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include <glm/gtx/transform.hpp>

#include <array>
#include <cmath>
#include <string>

namespace minire::utils
{
    class Aabb
    {
    public:
        Aabb(): _min(0), _max(0) {}

        explicit Aabb(float x, float y, float z,
                      float X, float Y, float Z)
            : _min(std::min(x, X), std::min(y, Y), std::min(z, Z))
            , _max(std::max(x, X), std::max(y, Y), std::max(z, Z))
        {}

        explicit Aabb(glm::vec3 const & min,
                      glm::vec3 const & max)
            : _min(min)
            , _max(max)
        {}

    public:
        glm::vec3 const & min() const { return _min; }

        glm::vec3 const & max() const { return _max; }

        glm::vec3 dims() const { return _max - _min; }

        void extend(glm::vec3 const & v)
        {
            _min = glm::min(_min, v);
            _max = glm::max(_max, v);
        }

        void extend(Aabb const & v)
        {
            _min = glm::min(_min, v._min);
            _max = glm::max(_max, v._max);
        }

        void transform(glm::mat4 const & transform, Aabb & out) const
        {
            std::array<glm::vec4, 8> const vertices {
                transform * glm::vec4(_min.x, _min.y, _min.z, 1.0f),
                transform * glm::vec4(_min.x, _min.y, _max.z, 1.0f),
                transform * glm::vec4(_min.x, _max.y, _min.z, 1.0f),
                transform * glm::vec4(_min.x, _max.y, _max.z, 1.0f),
                transform * glm::vec4(_max.x, _min.y, _min.z, 1.0f),
                transform * glm::vec4(_max.x, _min.y, _max.z, 1.0f),
                transform * glm::vec4(_max.x, _max.y, _min.z, 1.0f),
                transform * glm::vec4(_max.x, _max.y, _max.z, 1.0f),
            };

            glm::vec4 min(vertices[0]);
            glm::vec4 max(vertices[0]);
            for(glm::vec4 const & i : vertices)
            {
                min = glm::min(min, i);
                max = glm::max(max, i);
            }

            out._min.x = min.x;
            out._min.y = min.y;
            out._min.z = min.z;
            out._max.x = max.x;
            out._max.y = max.y;
            out._max.z = max.z;
        }

    private:
        glm::vec3 _min;
        glm::vec3 _max;
    };
}
