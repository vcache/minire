#pragma once

#include <minire/models/interpolation.hpp>
#include <minire/models/scene-path.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace minire::models
{
    struct KeyframeAnimation
    {
        using Timeline = std::shared_ptr<std::vector<float> const>;

        template<typename T>
        struct Track
        {
            using ValueType = T;

            std::shared_ptr<std::vector<T> const> _keyframes;
            models::Interpolation                 _interpolation;

            Track(std::shared_ptr<std::vector<T> const> keyframes,
                  models::Interpolation                 interpolation)
                : _keyframes(keyframes)
                , _interpolation(interpolation)
            {}
        };

        template<typename T>
        using MaybeTrack = std::optional<Track<T>>;

        using TranslationTrack = MaybeTrack<glm::vec3>;
        using RotationTrack = MaybeTrack<glm::quat>;
        using ScaleTrack = MaybeTrack<glm::vec3>;

        Timeline         _timeline;    // seconds

        TranslationTrack _translation;
        RotationTrack    _rotation;
        ScaleTrack       _scale;

        // NOTE: In case of kCubic, Keyframes must contain
        //       an in-tangent, a spline vertex, and an out-tangent
        //       per Timeline element. See glTF specification for detail.

        // NOTE: a target may have several animations, but only one
        //       can be playing at any given moment.
    };

    /* NOTE: the path is relative to a node with AnimationSet */
    using AnimationTracks = std::unordered_map<ScenePath,
                                               KeyframeAnimation>;

    using AnimationId = std::string;

    using AnimationSet = std::unordered_map<AnimationId, AnimationTracks>;
}