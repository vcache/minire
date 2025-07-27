#pragma once

#include <minire/models/transformation.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cmath>

namespace minire::grips
{
    // TODO: screen resoution to sensitivity
    // TODO: limits & sensevity
    // TODO: add panning
    // TODO: helpers for events::application::* processing and active camera
    class Orbiting
    {
    public:
        Orbiting(glm::vec3 target, float distance)
            : _target(target)
            , _distance(distance)
        {}

        void evaluate(models::Transformation & output)
        {
            static const glm::vec3 kUp(0.0f, 1.0f, 0.0f);

            output._translation.x = _distance * std::cos(_thetha) * std::cos(_phi);
            output._translation.y = _distance * std::sin(_phi);
            output._translation.z = _distance * std::sin(_thetha) * std::cos(_phi);

            glm::mat4 transform = glm::lookAt(output._translation, _target, kUp);
            output._rotation = glm::toQuat(glm::inverse(transform));
        }

        void updateDistance(float dDistance)
        {
            _distance += dDistance;
            _distance = glm::max<float>(0.0f, _distance);
        }

        void updateAngles(float dThetha, float dPhi)
        {
            _thetha += dThetha;
            _phi += dPhi;

            _phi = glm::clamp<float>(_phi, -M_PI/2.0f + std::numeric_limits<float>::epsilon(),
                                            M_PI/2.0f - std::numeric_limits<float>::epsilon());
        }

    private:
        glm::vec3 _target;
        float     _distance;
        float     _thetha = 0;
        float     _phi = 0;
    };
}