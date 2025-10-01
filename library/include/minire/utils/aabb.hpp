#pragma once

#include <minire/logging/formatters.hpp>

#include <fmt/format.h>
#include <glm/common.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/vec3.hpp>

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

        void transform(glm::mat4 const & transform)
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

            _min = vertices[0];
            _max = vertices[0];
            for(glm::vec4 const & i : vertices)
            {
                glm::vec3 vtx(i);
                _min = glm::min(_min, vtx);
                _max = glm::max(_max, vtx);
            }
        }

    private:
        glm::vec3 _min;
        glm::vec3 _max;
    };
}

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_same_v<T, ::minire::utils::Aabb>, char>>
    : fmt::formatter<std::string>
{
    template <typename FormatCtx>
    auto format(T const & value, FormatCtx & ctx) const
    {
        return fmt::formatter<std::string>::format(
            fmt::format("{} - {}", value.min(), value.max()), ctx);
    }
};
