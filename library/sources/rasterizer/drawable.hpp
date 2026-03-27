#pragma once

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <utility>
#include <vector>

namespace minire::rasterizer
{
    class Drawable
    {
    public:
        // Assuming these pointers won't be stored anywhere,
        // and nothing but draw() will be called.
        using PtrsList = std::vector<Drawable *>;

    public:
        virtual ~Drawable() = default;

        virtual void draw(glm::mat4 const & projection) = 0;

    public:
        size_t effectiveZOrder() const { return _effectiveZOrder; }

        void setEffectiveZOrder(size_t z) { _effectiveZOrder = z; }

    private:
        size_t _effectiveZOrder = 0;
    };
}
