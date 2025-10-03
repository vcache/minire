#pragma once

#include <minire/errors.hpp>

#include <glm/common.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/vec3.hpp>

namespace minire::models
{
    struct Transform
    {
        glm::vec3 _translation;
        glm::quat _rotation;
        glm::vec3 _scale;

        explicit Transform(glm::vec3 const & translation = glm::vec3(0.0f),
                           glm::quat const & rotation = glm::quat_identity<float, glm::defaultp>(),
                           glm::vec3 const & scale = glm::vec3(1.0f))
            : _translation(translation)
            , _rotation(rotation)
            , _scale(scale)
        {}

        void lerp(Transform const & prev,
                  Transform const & last,
                  float const weight)
        {
            // TODO: shouldn't lerp components that isn't changed

            _translation = glm::mix(prev._translation,  last._translation,   weight);
            _rotation    = glm::slerp(prev._rotation,   last._rotation,      weight);
            _scale       = glm::mix(prev._scale,        last._scale,         weight);
        }

        void loadFromMatrix(glm::mat4 const & matrix)
        {
            glm::vec3 skew;
            glm::vec4 perspective;
            bool success = glm::decompose(matrix,
                                          _scale,
                                          _rotation,
                                          _translation,
                                          skew,
                                          perspective);
            MINIRE_INVARIANT(success, "failed to decompose matrix");
#           if GLM_VERSION < 999
            _rotation = glm::conjugate(_rotation);
#           endif
        }

        glm::mat4 matrix() const
        {
            return glm::translate(_translation) *
                   glm::toMat4(_rotation) *
                   glm::scale(_scale);
        }

        bool operator==(Transform const &) const = default;
    };
}
