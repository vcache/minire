#include <minire/utils/ray-caster.hpp>

namespace minire::utils
{
    RayCaster::RayCaster(size_t const width,
                         size_t const height,
                         glm::mat4 const & view,
                         glm::mat4 const & projection)
    : _viewport(0.0f, 0.0f,
                static_cast<float>(width),
                static_cast<float>(height))
    , _view(view)
    , _projection(projection)
    {}

    Ray RayCaster::cast(glm::vec2 const & point) const
    {
        glm::vec3 win(point.x, _viewport.w - point.y, 0.0f);
        glm::vec3 const v0 = glm::unProject(win, // near
                                            _view,
                                            _projection,
                                            _viewport);
        win.z = 1.0f;
        glm::vec3 const v1 = glm::unProject(win, // far
                                            _view,
                                            _projection,
                                            _viewport);
        return Ray
        {
            ._origin = v0,
            ._direction = glm::normalize(v1 - v0)
        };
    }
}