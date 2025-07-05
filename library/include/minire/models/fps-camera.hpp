#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/common.hpp>

namespace minire::models
{
    class FpsCamera
    {
    public:
        FpsCamera(glm::vec3 const & position = glm::vec3(0.0f),
                  glm::vec3 const & front = glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec2 const & yawPitch = glm::vec2(-90.0f, 0.0f))
            : _position(position)
            , _front(front)
            , _yawPitch(yawPitch)
        {
            setYawPitch(_yawPitch); // just to make sure that
                                    // _front and _yawPitch
                                    // are in sync
        }

    public:
        void forward(float const delta)
        {
            _position += _front * delta;
        }

        void strafe(float const delta)
        {
            _position += glm::normalize(
                glm::cross(_front, up())
            ) * delta;
        }

        void astrafe(float const delta)
        {
            auto const & side = glm::cross(_front, up());
            _position += glm::normalize(
                glm::cross(up(), side)
            ) * delta;
        }

        void setYawPitch(glm::vec2 const & yawPitch)
        {
            _yawPitch.x = yawPitch.x;
            //_yawPitch.y = glm::clamp(yawPitch.y, -89.0f, 89.0f);
            _yawPitch.y = glm::clamp(yawPitch.y, -89.0f, -23.0f); // TODO: what kind of magic it is?

            auto const yaw = _yawPitch.x;
            auto const pitch = _yawPitch.y;

            glm::vec3 dir;
            dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            dir.y = sin(glm::radians(pitch));
            dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            _front = glm::normalize(dir);
        }

        void addYawPitch(glm::vec2 const & delta)
        {
            setYawPitch(_yawPitch + delta);
        }

        void lerp(FpsCamera const & prev,
                  FpsCamera const & last,
                  float const weight)
        {
            _position = glm::mix(prev._position, last._position, weight);
            _front    = glm::mix(prev._front,    last._front,    weight);
            _yawPitch = glm::mix(prev._yawPitch, last._yawPitch, weight);
        }

    public:
        glm::mat4 view() const
        {
            return glm::lookAt(_position,
                               _position + _front,
                               up());
        }

        glm::vec3 const & position() const { return _position; }

        glm::vec3 const & front() const { return _front; }

        glm::vec2 const & yawPitch() const { return _yawPitch; }

    private:
        glm::vec3 const & up() const
        {
            static const glm::vec3 kUp(0.0f, 1.0f, 0.0f);
            return kUp;
        }

    private:
        glm::vec3 _position;
        glm::vec3 _front;
        glm::vec2 _yawPitch; // (yaw, pitch)

        friend void lerp(FpsCamera const &,
                         FpsCamera const &,
                         float const,
                         FpsCamera &);
    };
}
