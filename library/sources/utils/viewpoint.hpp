#pragma once

#include <utils/cull-frustum.hpp>

#include <glm/vec3.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/common.hpp>
//#include <glm/gtx/quaternion.hpp>

namespace minire::utils
{
    class Viewpoint
    {
    public:
        Viewpoint()
            : _position(0, 0, 0)
            , _projection(glm::identity<glm::mat4>())
            , _view(glm::identity<glm::mat4>())
            , _window(0.0f)
            , _transform(glm::identity<glm::mat4>())
            , _flags(kTransformInvalidated)
        {}

    public:
        auto const & cullFrustum() const { return _cullFrustum; }

        glm::mat4 const & projection() const { return _projection; }

        glm::mat4 const & view() const { return _view; }

        // NOTE: unless this method is const it is  not thread-safe
        glm::mat4 const & transform() const
        {
            revalidate();
            return _transform;
        }

        glm::mat4 const & invTransform() const
        {
            revalidate();
            return _invTransform;
        }

        glm::vec3 const & position() const { return _position; }

        size_t transformVersion() const { return _transformVersion; }

        glm::vec4 const & window() const { return _window; }

    public:
        void setProjection(glm::mat4 const & projection,
                           float const width,
                           float const height)
        {
            _projection = projection;
            _window.x = _window.y = 0.0f;
            _window.z = width;
            _window.w = height;
            invalidate();
        }

        void setView(glm::mat4 const & view,
                     glm::vec3 const & position)
        {
            _view = view;
            _position = position;
            invalidate();
        }

    private:
        void invalidate()
        {
            _flags |= kTransformInvalidated;
        }

        void revalidate() const
        {
            if (_flags & kTransformInvalidated)
            {
                _transform = _projection * _view;
                _invTransform = glm::inverse(_transform);
                ++_transformVersion;

                utils::frustumFromMatrix(_transform, _cullFrustum, true);

                _flags &= ~kTransformInvalidated;
            }
        }

    private:
        glm::vec3              _position;
        glm::mat4              _projection;
        glm::mat4              _view;
        glm::vec4              _window;

        mutable utils::Frustum _cullFrustum;
        mutable glm::mat4      _transform;
        mutable glm::mat4      _invTransform;
        mutable size_t         _transformVersion = 0;
        mutable uint64_t       _flags;

    private:
        static constexpr uint64_t kTransformInvalidated = (1 << 0);
    };

    // NOTE MVPmatrix = projection * view * model;
    // i.e. in shader: gl_Position = viewpoint.transform() * model.transform() * inPos;

    // https://learnopengl.com/Getting-started/Coordinate-Systems
    // http://www.codinglabs.net/article_world_view_projection_matrix.aspx
    // http://www.opengl-tutorial.org/ru/beginners-tutorials/tutorial-3-matrices/
}
