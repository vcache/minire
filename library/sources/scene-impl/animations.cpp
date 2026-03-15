#include <scene-impl/animations.hpp>

#include <minire/errors.hpp>
#include <minire/models/animations.hpp>

#include <cassert>

namespace minire::scene
{
    namespace
    {
        template<typename T>
        void validateTrack(models::KeyframeAnimation::MaybeTrack<T> const & in,
                           models::KeyframeAnimation::Timeline const & timeline,
                           char const * itemName)
        {
            assert(timeline);
            MINIRE_INVARIANT(!in || (in->_keyframes && in->_keyframes->size() == timeline->size()),
                             "timeline and {} keyframes mismatch: {} != {}",
                             itemName,
                             in ? (in->_keyframes ? in->_keyframes->size() : -1) : -2,
                             timeline->size());
        }
    }

    KeyframeAnimation::Sptr
    makeKeyframeAnimation(models::KeyframeAnimation const & input)
    {
        MINIRE_INVARIANT(input._timeline && !input._timeline->empty(),
                         "animation timeline is empty");

        validateTrack(input._translation, input._timeline, "translation");
        validateTrack(input._rotation, input._timeline, "rotation");
        validateTrack(input._scale, input._timeline, "scale");

        KeyframeAnimation::Sptr result = std::make_shared<KeyframeAnimation>();

        if (input._translation)
        {
            result->_translation = std::make_shared<KeyframeAnimation::TranslationCurve>(
                input._translation->_interpolation, input._timeline, input._translation->_keyframes);
        }

        if (input._rotation)
        {
            result->_rotation = std::make_shared<KeyframeAnimation::RotationCurve>(
                input._rotation->_interpolation, input._timeline, input._rotation->_keyframes);
        }

        if (input._scale)
        {
            result->_scale = std::make_shared<KeyframeAnimation::ScaleCurve>(
                input._scale->_interpolation, input._timeline, input._scale->_keyframes);
        }

        return result;
    }
}
