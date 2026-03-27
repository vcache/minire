#pragma once

#include <minire/models/transform.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cassert>
#include <cmath>

namespace minire::grips
{
    // TODO: screen resoution to sensitivity
    // TODO: helpers for events::* processing and active camera
    class Orbiting
    {
        constexpr float static kEpsilon = std::numeric_limits<float>::epsilon();
        constexpr glm::vec2 static kPhyHardLimit = glm::vec2(-M_PI/2.0f + kEpsilon,
                                                              M_PI/2.0f - kEpsilon);

    public:
        Orbiting(glm::vec3 target, float distance,
                 float thetha = 0, float phi = 0,
                 glm::vec2 phiMinMax = kPhyHardLimit)
            : _target(target)
            , _distance(distance)
            , _thetha(thetha)
            , _phi(phi)
            , _phiMinMax(std::max(phiMinMax[0], kPhyHardLimit[0]),
                         std::min(phiMinMax[1], kPhyHardLimit[1]))
        {
            assert(_phiMinMax[0] <= _phiMinMax[1]);
        }

        void evaluate(models::Transform & output)
        {
            static constexpr glm::vec3 kUp(0.0f, 1.0f, 0.0f);

            output._translation.x = _distance * std::cos(_thetha) * std::cos(_phi);
            output._translation.y = _distance * std::sin(_phi);
            output._translation.z = _distance * std::sin(_thetha) * std::cos(_phi);

            output._translation += _target;

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

            _phi = glm::clamp<float>(_phi, _phiMinMax[0], _phiMinMax[1]);
        }

        glm::vec3 & target() { return _target; }

        glm::vec3 const & target() const { return _target; }

        float distance() const { return _distance; }

        Orbiting & operator=(Orbiting const &) = default;

    private:
        glm::vec3 _target;
        float     _distance;
        float     _thetha;
        float     _phi;
        glm::vec2 _phiMinMax;
    };
}
