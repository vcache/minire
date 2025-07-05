#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace minire::models
{
    /*
        Range   Constant    Linear  Quadratic
        3250,   1.0,        0.0014, 0.000007
        600,    1.0,        0.007,  0.0002
        325,    1.0,        0.014,  0.0007
        200,    1.0,        0.022,  0.0019
        160,    1.0,        0.027,  0.0028
        100,    1.0,        0.045,  0.0075
        65,     1.0,        0.07,   0.017
        50,     1.0,        0.09,   0.032
        32,     1.0,        0.14,   0.07
        20,     1.0,        0.22,   0.20
        13,     1.0,        0.35,   0.44
        7,      1.0,        0.7,    1.8 
    */

    struct PointLight
    {
        glm::vec4 _origin;
        glm::vec4 _color; // rgb + w is intesity
        glm::vec4 _attenuation;

        PointLight(glm::vec3 const & origin,
                   glm::vec4 const & color,
                   glm::vec3 const & attenuation)
            : _origin(origin, 1.0f)
            , _color(color)
            , _attenuation(attenuation, 0.0f)
        {}

        PointLight(glm::vec3 const & origin,
                   glm::vec4 const & color,
                   float range)
            : _origin(origin, 1.0f)
            , _color(color)
            , _attenuation(attFromRange(range), 0.0f)
        {}

        static glm::vec3 attFromRange(float range)
        {
            return glm::vec3(1.0f, 4.5f / range, 75.0f / (range*range));
        }

        void lerp(PointLight const & prev,
                  PointLight const & last,
                  float const weight)
        {
            _origin      = glm::mix(prev._origin,      last._origin,      weight);
            _color       = glm::mix(prev._color,       last._color,       weight);
            _attenuation = glm::mix(prev._attenuation, last._attenuation, weight);
        }
    };
}
