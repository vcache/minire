#pragma once

#include <minire/models/transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cassert>

namespace minire::grips
{
    // TODO: camera's fov/zoom and screen resolution should be taken into account (instead "float scale")
    //       to provide uniform panning movement (i.e. the pixel under a cursor must remain the same)
    class ScreenSpacePanning
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

        glm::vec3 update(size_t mouseX, size_t mouseY,
                         glm::mat4 const & matrix,
                         glm::vec3 const & target,
                         float scale = 0.01f)
        {
            assert(isActive());
            if (isActive())
            {
                glm::vec2 const screenPos{static_cast<float>(mouseX),
                                          static_cast<float>(mouseY)};
                glm::vec2 const screenDelta = screenPos - _begin;

                _offset = (glm::vec3(matrix[0]) * (-screenDelta.x) +
                           glm::vec3(matrix[1])* (screenDelta.y)) * scale;
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
        glm::vec2 _begin = glm::vec2(0.0f);   // screen space
        glm::vec3 _offset = glm::vec3(0.0f);  // world space
        bool      _active = false;
    };
}