#pragma once

#include <minire/content/path.hpp>
#include <minire/errors.hpp>

#include <utils/curve.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <memory>

// TODO: shouldn't it be a part of a content::Manager?

namespace minire::models { class KeyframeAnimation; }

namespace minire::scene
{
    struct KeyframeAnimation
    {
        using Sptr = std::shared_ptr<KeyframeAnimation>;
        using TranslationCurve = utils::Curve<float, glm::vec3>;
        using RotationCurve = utils::Curve<float, glm::quat>;
        using ScaleCurve = utils::Curve<float, glm::vec3>;

        TranslationCurve::Sptr _translation;
        RotationCurve::Sptr    _rotation;
        ScaleCurve::Sptr       _scale;
    };

    KeyframeAnimation::Sptr makeKeyframeAnimation(models::KeyframeAnimation const &);
}