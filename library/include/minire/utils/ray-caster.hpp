#pragma once

#include <minire/utils/geometry.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <memory>

namespace minire::utils
{
    class RayCaster
    {
    public:
        using Uptr = std::unique_ptr<RayCaster>;

        RayCaster(size_t const width,
                  size_t const height,
                  glm::mat4 const & view,
                  glm::mat4 const & projection);

        // \a point is a screen-space point in pixels
        Ray cast(glm::vec2 const & point) const;

    private:
        glm::vec4 const _viewport; // (0, 0, width, height)
        glm::mat4 const _view;
        glm::mat4 const _projection;
    };
}