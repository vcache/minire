#include <scene/viewpoint.hpp>

#include <minire/errors.hpp>

#include <utils/overloaded.hpp>

// NOTE MVPmatrix = projection * view * model;
// i.e. in shader: gl_Position = viewpoint.transform() * model.transform() * inPos;

// https://learnopengl.com/Getting-started/Coordinate-Systems
// http://www.codinglabs.net/article_world_view_projection_matrix.aspx
// http://www.opengl-tutorial.org/ru/beginners-tutorials/tutorial-3-matrices/

namespace minire::scene
{
    void Viewpoint::setViewport(size_t const width,
                                size_t const height)
    {
        if (_vpWidth != width || _vpHeight != height)
        {
            _vpWidth = width;
            _vpHeight = height;
            recalcProjection();
        }
    }

    void Viewpoint::setTransform(glm::mat4 const & view,
                                 glm::vec3 const & position)
    {
        glm::mat4 inverseView = glm::inverse(view);
        if (_position != position || inverseView != _view)
        {
            _position = position;
            _view = inverseView;
            invalidate();
        }
    }

    void Viewpoint::unsetCamera()
    {
        _camera = std::monostate();
    }

    void Viewpoint::setCamera(models::PerspectiveCamera const & perspectiveCamera)
    {
        if (!isSame(perspectiveCamera))
        {
            _camera = perspectiveCamera;
            recalcProjection();
        }
    }

    void Viewpoint::setCamera(models::OrthographicCamera const & orthographicCamera)
    {
        if (!isSame(orthographicCamera))
        {
            _camera = orthographicCamera;
            recalcProjection();
        }
    }

    bool Viewpoint::isSame(models::PerspectiveCamera const & camera) const
    {
        return std::holds_alternative<models::PerspectiveCamera>(_camera)
            && std::get<models::PerspectiveCamera>(_camera) == camera;
    }

    bool Viewpoint::isSame(models::OrthographicCamera const & camera) const
    {
        return std::holds_alternative<models::OrthographicCamera>(_camera)
            && std::get<models::OrthographicCamera>(_camera) == camera;
    }

    void Viewpoint::revalidate() const
    {
        if (_invalidated)
        {
            _mvp = _projection * _view;
            _revision++;
            _invalidated = false;
        }
    }

    void Viewpoint::invalidate()
    {
        _invalidated = true;
    }

    void Viewpoint::recalcProjection()
    {
        std::visit(utils::Overloaded
        {
            [](std::monostate) {},
            [this](models::PerspectiveCamera const & camera)
            {
                float const aspectRatio = camera._aspectRatio
                    ? *camera._aspectRatio
                    : static_cast<float>(_vpWidth) / static_cast<float>(_vpHeight);

                if (camera._zFar)
                {
                    _projection = glm::perspective(camera._yFov, aspectRatio,
                                                   camera._zNear, *camera._zFar);
                }
                else
                {
                    _projection = glm::infinitePerspective(camera._yFov, aspectRatio,
                                                           camera._zNear);
                }
                invalidate();
            },
            [this](models::OrthographicCamera const &)
            {
                MINIRE_THROW("orthographic camera isn't implemented");
                /*
                float const halfWidth = static_cast<float>(_vpWidth) / 2.0f;
                float const halfHeight = static_cast<float>(_vpHeight) / 2.0f;

                //pmat = glm::ortho(0.0f, fWidth, 0.0f, fHeight, kNear, kFar);
                _projection = glm::ortho(-halfWidth * camera._xMag,
                                          halfWidth * camera._xMag,
                                         -halfHeight * camera._yMag,
                                          halfHeight * camera._yMag,
                                          camera._zNear, camera._zFar);

                float const ratio = fWidth / fHeight;
                float const scale = 10.0f;
                pmat = glm::ortho(-scale, scale,
                                  -scale * ratio, scale * ratio,
                                  kNear, kFar);
                invalidate();
                */
            }
        }, _camera);
    }
}
