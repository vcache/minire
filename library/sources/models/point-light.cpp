#include <minire/models/point-light.hpp>

#include <algorithm>
#include <cmath>

namespace minire::models
{
    std::optional<float> PointLight::maxRadius() const
    {
        // the brightest color channel multiplied by intensity
        float const maxChannel = std::max({_color.x, _color.y, _color.z});
        float const maxIntensity = maxChannel * _color.w;

        // early quit for dim lights
        if (maxIntensity <= 0.0f) return 0.0f;

        // 5/256 is a standard default
        float const threshold = 5.0f / 256.0f;

        // attenuation coefficients
        float const Kc = _attenuation.x;
        float const Kl = _attenuation.y;
        float const Kq = _attenuation.z;

        // the equation: Kq*d^2 + Kl*d + (Kc - I/Threshold) = 0
        float const c = Kc - (maxIntensity / threshold);

        if (Kq > 0.00001f)
        {
            // quadratic attenuation is present (Standard)
            if (float const discriminant = (Kl * Kl) - (4.0f * Kq * c);
                discriminant >= 0.0f)
            {
                // only the positive root
                return (-Kl + std::sqrt(discriminant)) / (2.0f * Kq);
            }
        }
        else if (Kl > 0.00001f)
        {
            // only Linear attenuation (No Quadratic)
            return -c / Kl;
        }

        // never truly fades
        return std::nullopt;
    }
}