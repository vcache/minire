#pragma once

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <vector>

namespace minire::gpu::render
{
    class Drawable
    {
    public:
        using PtrsList = std::vector<Drawable const *>;

    public:
        explicit Drawable(size_t zOrder)
            : _zOrder(zOrder)
        {}

        virtual ~Drawable() = default;

        virtual void draw(glm::mat4 const & projection) const = 0;

    public:
        size_t zOrder() const { return _zOrder; }

        void setZOrder(size_t z) { _zOrder = z; }

    private:
        size_t _zOrder;
    };
}
