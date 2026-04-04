#pragma once

#include <optional>
#include <variant>

namespace minire::models
{
    namespace shadow_params
    {
        namespace bias
        {
            struct Constant
            {
                float _biasBase = 0.0005f;

                bool operator==(Constant const &) const = default;
            };

            struct SlopScaled
            {
                float _biasBase = 0.0005f;
                float _maxBias  = 0.005f;

                bool operator==(SlopScaled const &) const = default;
            };
        }

        using Bias = std::variant<bias::Constant,
                                  bias::SlopScaled>;

        namespace method
        {
            // Naive shadow mapping based on z-buffer values
            struct Standard
            {
                bool operator==(Standard const &) const = default;
            };

            // Exponential Shadow Maps,
            // the map stores exp(K*z) with linearized z
            struct ESM
            {
                float _factor = 1.0f; // K

                bool operator==(ESM const &) const = default;
            };

            // Logarithmic-Exponential Shadow Maps,
            // the map stores K*z with linearized z
            struct LogESM
            {
                float _factor = 1.0f; // K

                bool operator==(LogESM const &) const = default;
            };
        }

        using Method = std::variant<method::Standard,
                                    method::ESM,
                                    method::LogESM>;

        // TODO: parametrize filters (different kernel sizes, for example)
        namespace filter
        {
            // Percentage-Closer Filtering
            struct PCF
            {
                bool operator==(PCF const &) const = default;
            };

            // Gaussian Blur with 9x9 kernel
            struct GaussianBlur
            {
                size_t _iterations = 1;

                bool operator==(GaussianBlur const &) const = default;
            };
        }

        using Filter = std::variant<std::monostate,
                                    filter::PCF,
                                    filter::GaussianBlur>;
        namespace margin
        {
            struct Absolute
            {
                float _value = 0.0f;

                bool operator==(Absolute const &) const = default;
            };

            struct Constant
            {
                float _value = 0.0f;

                bool operator==(Constant const &) const = default;
            };

            struct Factor
            {
                float _value = 1.0f;

                bool operator==(Factor const &) const = default;
            };
        }

        using Margin = std::variant<std::monostate,
                                    margin::Absolute,
                                    margin::Constant,
                                    margin::Factor>;

        namespace center
        {
            struct Absolute
            {
                glm::vec3 _value;

                bool operator==(Absolute const &) const = default;
            };

            // the default choice for most applications
            struct Frustum
            {
                bool operator==(Frustum const &) const = default;
            };

            // A point on a Y-Plane (Y=0) where camera direction intersects
            // with this plane. Might be useful for top-view cameras,
            // like RTS games or isometric views. With this option,
            // a _radiusMargin should be tweaked as well.
            // Applicable only for directional lights.
            struct CameraYPlaneHitPoint
            {
                bool operator==(CameraYPlaneHitPoint const &) const = default;
            };
        }

        using Center = std::variant<center::Absolute,
                                    center::Frustum,
                                    center::CameraYPlaneHitPoint>;
    }

    struct ShadowParams
    {
        size_t                   _mapSize        = 1024;
        shadow_params::Method    _method         = shadow_params::method::Standard{};
        shadow_params::Filter    _filter         = std::monostate();
        shadow_params::Bias      _normalBias     = shadow_params::bias::Constant{};
        shadow_params::Bias      _depthBias      = shadow_params::bias::Constant{};
        shadow_params::Margin    _nearMargin     = std::monostate();
        shadow_params::Margin    _farMargin      = std::monostate();
        shadow_params::Margin    _radiusMargin   = std::monostate();
        shadow_params::Center    _center         = shadow_params::center::Frustum{};
        std::pair<float, float>  _smoothStep     = std::pair(0.0f, 1.0f); // increase "first" to reduce light bleeding
        bool                     _zBuffer32      = true; // use 32-bit float for depth map component

        // TODO: implement shadow groups (maybe it should be a bitset?)

        bool operator==(ShadowParams const &) const = default;
    };

    using MaybeShadowParams = std::optional<ShadowParams>;
}
