#pragma once

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

namespace minire::models
{
    struct ModelPosition
    {
        glm::vec3 _origin;
        glm::quat _rotation;

        explicit ModelPosition(glm::vec3 const & origin = glm::vec3(0.0f),
                               glm::quat const & rotation = glm::quat_identity<float, glm::defaultp>())
            : _origin(origin)
            , _rotation(rotation)
        {}

        void lerp(ModelPosition const & prev,
                  ModelPosition const & last,
                  float const weight)
        {
            _origin   = glm::mix(prev._origin,   last._origin,   weight);
            _rotation = glm::slerp(prev._rotation, last._rotation, weight);
        }
    };
}
