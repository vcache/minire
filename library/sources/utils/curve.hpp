#pragma once

#include <minire/errors.hpp>
#include <minire/models/interpolation.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/spline.hpp>

#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

namespace minire::utils
{
    // TODO: maybe move it into the public API?
    template<typename Input,
             typename Output>
    class Curve
    {
    public:
        using Sptr = std::shared_ptr<Curve>;
        using Inputs = std::shared_ptr<std::vector<Input> const>;
        using Outputs = std::shared_ptr<std::vector<Output> const>;

        Curve(models::Interpolation interpolation,
              Inputs inputs,
              Outputs outputs)
            : _interpolation(interpolation)
            , _inputs(inputs)
            , _outputs(outputs)
        {
            assert(_inputs);
            assert(_outputs);

            size_t const kOutputStride = interpolation == models::Interpolation::kCubic ? 3 : 1;
            MINIRE_INVARIANT(_inputs->size() == _outputs->size() * kOutputStride,
                             "inputs/outputs size mismatch ({} != {}, {})",
                             _inputs->size(), _outputs->size(), kOutputStride);
            MINIRE_INVARIANT(!_inputs->empty(), "no points provided");
        }

        Inputs const & inputs() const { return _inputs; }

        Output interpolate(size_t index, Input timestamp) const
        {
            if (index >= _inputs->size() - 1)
            {
                if (_interpolation == models::Interpolation::kCubic)
                {
                    return (*_outputs)[1 + (_inputs->size() - 1) * 3];
                }
                else
                {
                    return (*_outputs)[_inputs->size() - 1];
                }
            }

            assert(index + 1 < _inputs->size());
            assert((*_inputs)[index] <= timestamp);
            assert(timestamp < (*_inputs)[index + 1]);

            // TODO: don't calc for kStep
            MINIRE_INVARIANT((*_inputs)[index + 1] != (*_inputs)[index],
                             "bad keyframe with same inputs: {}", (*_inputs)[index]);
            float const duration = (*_inputs)[index + 1] - (*_inputs)[index];
            float const normalTs = (timestamp - (*_inputs)[index]) / duration;
            assert(0.0f <= normalTs && normalTs <= 1.0f);

            switch(_interpolation)
            {
                case models::Interpolation::kStep:
                    return (*_outputs)[index];

                case models::Interpolation::kLinear:
                    if constexpr(std::is_same_v<Output, glm::quat>)
                    {
                        // TODO: When a is close to zero, spherical linear interpolation
                        //       turns into regular linear interpolation.
                        return glm::slerp((*_outputs)[index], (*_outputs)[index + 1], normalTs);
                    }
                    else
                    {
                        return glm::mix((*_outputs)[index], (*_outputs)[index + 1], normalTs);
                    }

                case models::Interpolation::kCubic:
                {
                    // For each input element, the output stores three elements,
                    // an in-tangent, a spline vertex, and an out-tangent.
                    auto result = glm::hermite((*_outputs)[index*3 + 1],                  // V_k,     k-th value
                                               (*_outputs)[index*3 + 2] * duration,       // b_k      k-th out-tangent
                                               (*_outputs)[(index+1)*3 + 1],              // V_{k+1}, k+1-th value
                                               (*_outputs)[(index+1)*3 + 0] * duration,   // a_{k+1}  k+1-th in-tangent
                                               normalTs);
                    if constexpr(std::is_same_v<Output, glm::quat>)
                    {
                        // When the animation sampler targets a node’s rotation property,
                        // the interpolated quaternion MUST be normalized before applying
                        // the result to the node’s rotation.
                        result = glm::normalize(result);
                    }

                    return result;
                }
            }

            MINIRE_THROW("unknown interpolatation mode: {}", static_cast<int>(_interpolation));
        }

        size_t size() const
        {
            assert(_inputs->size() == _outputs->size());
            return _inputs->size();
        }

    private:
        models::Interpolation const _interpolation;
        Inputs                      _inputs;
        Outputs                     _outputs;
    };

    template<typename Input>
    class Sequencer
    {
    public:
        using Timeline = std::shared_ptr<std::vector<Input> const>;
        using Sptr = std::shared_ptr<Sequencer>;
        using CSptr = std::shared_ptr<Sequencer const>;

        static constexpr size_t kInfinitely = std::numeric_limits<size_t>::max();

        explicit Sequencer(Timeline timeline,
                           size_t const repeats,
                           Input const speedScale)
            : _timeline(timeline)
            , _index(0)
            , _timestamp(Input())
            , _repeats(repeats)
            , _speedScale(speedScale)
        {
            MINIRE_INVARIANT(_timeline, "timeline isn't specified");
            MINIRE_INVARIANT(!_timeline->empty(), "timeline is empty");
        }

        template<typename Output>
        Output current(Curve<Input, Output> const & curve) const
        {
            assert(curve.inputs());
            assert(curve.inputs()->size() == _timeline->size());
            return curve.interpolate(_index, _timestamp);
        }

        void advance(Input delta)
        {
            assert(_timeline);
            if (_repeats == 0) return;

            _timestamp += delta * _speedScale;
            _timestamp = std::min(_timestamp, _timeline->back());
            while(_index + 1 < _timeline->size() &&
                  _timestamp >= (*_timeline)[_index + 1])
            {
                ++_index;
            }

            assert(_index <= _timeline->size() - 1);
            if (_index == _timeline->size() - 1)
            {
                if (_repeats != kInfinitely) _repeats--;
                if (_repeats != 0)
                {
                    _index = 0;
                    _timestamp = _timestamp - _timeline->back();
                    assert(_timestamp >= 0.0f);
                }
            }
        }

        bool isDone() const
        {
            return _repeats == 0;
        }

        Timeline const & timeline() const { return _timeline; }

    private:
        Timeline    _timeline;
        size_t      _index;
        Input       _timestamp;
        size_t      _repeats;
        Input const _speedScale;
    };
}