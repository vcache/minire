#include <scene-impl/viewpoint.hpp>

#include <minire/errors.hpp>
#include <minire/utils/overloaded.hpp>

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

    // TODO: calculate it lazily
    utils::ViewFrustum Viewpoint::viewFrustum() const
    {
        revalidate();

        glm::mat4 const inversed = glm::inverse(_projection * _view);
        auto invProject = [&inversed](glm::vec4 const & ndcVertex)
        {
            // TODO: what if worldVertex.w == 0
            glm::vec4 worldVertex = inversed * ndcVertex;
            return glm::vec3(worldVertex.x / worldVertex.w,
                             worldVertex.y / worldVertex.w,
                             worldVertex.z / worldVertex.w);
        };

        // NOTE: some code is relying on this specific order of vertices,
        //       they must be changed. See cullingTest() function.
        return utils::ViewFrustum
        {
            ._vertices = {
                // Near
                invProject(glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f)),   // bottom-left
                invProject(glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f)),   // top-left
                invProject(glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f)),   // top-right
                invProject(glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f)),   // bottom-right

                // Far
                invProject(glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f)),   // bottom-left
                invProject(glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f)),   // top-left
                invProject(glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f)),   // top-right
                invProject(glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f)),   // bottom-right
            },
            ._nearPlane = inversed * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f),
            ._farPlane  = inversed * glm::vec4(0.0f, 0.0f,  1.0f, 1.0F),
        };
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
            [this](models::OrthographicCamera const & camera)
            {
                float const aspectRatio = static_cast<float>(_vpHeight) /
                                          static_cast<float>(_vpWidth);
                _projection = glm::ortho(-camera._xMag,
                                          camera._xMag,
                                         -camera._yMag * aspectRatio,   // NOTE: aspectRatio is implictly multiplied
                                          camera._yMag * aspectRatio,   //       by 1.0 which is the "width"
                                          camera._zNear, camera._zFar);

                invalidate();
            }
        }, _camera);
    }

    glm::vec3 Viewpoint::forwardVector() const
    {
        return glm::normalize(glm::vec3(-_view[0][2], -_view[1][2], -_view[2][2]));
    }
}
