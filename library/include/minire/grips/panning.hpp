#pragma once

#include <minire/models/camera.hpp>
#include <minire/models/transform.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cassert>
#include <cmath>

namespace minire::grips
{
    template<bool kScreenSpace>
    class Panning
    {
    public:
        void start(size_t mouseX, size_t mouseY)
        {
            assert(!isActive());
            if (!isActive())
            {
                _begin = glm::vec2(static_cast<float>(mouseX),
                                   static_cast<float>(mouseY)),
                _offset = glm::vec3(0.0f);
                _active = true;
            }
        }

        template<typename Camera>
        glm::vec3 update(size_t const mouseX,
                         size_t const mouseY,
                         glm::mat4 const & matrix,
                         glm::vec3 const & target,
                         Camera const & camera,
                         glm::vec2 const & windowSize,
                         glm::vec3 const & cameraPosition)
        {
            assert(isActive());
            if (isActive())
            {
                glm::vec2 const screenPos{static_cast<float>(mouseX),
                                          static_cast<float>(mouseY)};
                glm::vec2 const screenDelta = screenPos - _begin;
                glm::vec2 const horVerScale =
                    scale(target, cameraPosition, camera, windowSize) *
                    screenDelta * glm::vec2(-1.0f, 1.0f);

                if constexpr (kScreenSpace)
                {
                    _offset = glm::vec3(matrix[0]) * horVerScale[0] +
                              glm::vec3(matrix[1]) * horVerScale[1];
                }
                else
                {
                    static constexpr glm::vec3 kUp(0.0f, 1.0f, 0.0f);
                    glm::vec3 const column0(matrix[0]);
                    _offset = column0 * horVerScale[0] +
                              glm::cross(kUp, column0) * horVerScale[1];
                }
            }
            return target + _offset;
        }

        void finish(glm::vec3 & target)
        {
            assert(isActive());
            if (isActive())
            {
                target += _offset;
                _offset = glm::vec3(0.0f);
                _active = false;
            }
        }

    public:
        bool isActive() const { return _active; }

        operator bool() const { return isActive(); }

    private:
        glm::vec2 scale(
            glm::vec3 const & target,
            glm::vec3 const & cameraPosition,
            models::PerspectiveCamera const & camera,
            glm::vec2 const & windowSize) const
        {
            float effectiveDistance = glm::length(target - cameraPosition);
            effectiveDistance *= std::tan(camera._yFov / 2.0f);
            return glm::vec2(2.0f * effectiveDistance / windowSize.y);
        }

        // TODO: implement for models::OrthographicCamera

    private:
        glm::vec2 _begin = glm::vec2(0.0f);   // screen space
        glm::vec3 _offset = glm::vec3(0.0f);  // world space
        bool      _active = false;
    };
}