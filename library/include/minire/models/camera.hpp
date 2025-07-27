#pragma once

#include <minire/utils/float-compare.hpp>

#include <glm/common.hpp>

#include <optional>

namespace minire::models
{
    struct PerspectiveCamera
    {
        float                _yFov;         // radians
        float                _zNear;
        std::optional<float> _zFar;         // infinite range when std::nullopt
        std::optional<float> _aspectRatio;  // viewport used when std::nullopt

        void lerp(PerspectiveCamera const & prev,
                  PerspectiveCamera const & last,
                  float const weight)
        {
            _yFov  = glm::mix(prev._yFov,  last._yFov,  weight);
        }

        bool operator==(PerspectiveCamera const & other) const
        {
            return utils::isNear(_yFov,        other._yFov)
                && utils::isNear(_zNear,       other._zNear)
                && utils::isNear(_zFar,        other._zFar)
                && utils::isNear(_aspectRatio, other._aspectRatio);
        }
    };

    struct OrthographicCamera
    {
        float _xMag;
        float _yMag;
        float _zNear;
        float _zFar;

        void lerp(OrthographicCamera const & prev,
                  OrthographicCamera const & last,
                  float const weight)
        {
            _xMag  = glm::mix(prev._xMag,  last._xMag,  weight);
            _yMag  = glm::mix(prev._yMag,  last._yMag,  weight);
        }

        bool operator==(OrthographicCamera const & other) const
        {
            return utils::isNear(_xMag,  other._xMag)
                && utils::isNear(_yMag,  other._yMag)
                && utils::isNear(_zNear, other._zNear)
                && utils::isNear(_zFar,  other._zFar);
        }
    };
}
