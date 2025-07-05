#pragma once

#include <minire/content/id.hpp>
#include <minire/models/model-position.hpp>
#include <minire/utils/aabb.hpp>

#include <utils/lerpable.hpp>

#include <glm/gtx/transform.hpp>

#include <array>
#include <memory>
#include <vector>

namespace minire::scene
{
    class Model
    {
        using Lerpable = utils::Lerpable<models::ModelPosition>;

    public:
        Model(size_t id,
              content::Id model,
              utils::Aabb const & aabb, // will store a reference!
              models::ModelPosition const & position)
            : _id(id)
            , _model(model)
            , _lerpable(position)
            , _aabb(aabb)
            , _aabbTransformed(aabb)
        {
            updateTransform();
        }

        bool lerp(float weight, size_t epochNumber)
        {
            bool result = _lerpable.lerp(weight, epochNumber);
            updateTransform();
            return result;
        }

        void update(size_t epochNumber,
                    models::ModelPosition const & position)
        {
            _lerpable.update(epochNumber, position);
        }

        glm::mat4 const & transform() const { return _transform; }

        content::Id const & model() const { return _model; }

        size_t id() const { return _id; }

        using Uptr = std::unique_ptr<Model>;

    private:
        void updateTransform()
        {
            auto const & current = _lerpable.current();
            _transform = glm::translate(current._origin) *
                         glm::toMat4(current._rotation);
            _aabb.transform(_transform, _aabbTransformed);
        }

    private:
        size_t              _id;
        content::Id         _model;
        Lerpable            _lerpable;
        glm::mat4           _transform;
        utils::Aabb const & _aabb;
        utils::Aabb         _aabbTransformed;
    };

    struct ModelRef
    {
        content::Id       _model;
        glm::mat4 const * _transform;
        float             _colorFactor;

        explicit ModelRef(content::Id model,
                          glm::mat4 const & transform,
                          float const colorFactor)
            : _model(model)
            , _transform(&transform)
            , _colorFactor(colorFactor)
        {}

        using List = std::vector<ModelRef>;
    };
}
