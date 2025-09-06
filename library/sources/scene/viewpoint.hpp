#pragma once

#include <minire/models/camera.hpp>
#include <minire/models/transform.hpp>

#include <glm/vec3.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/common.hpp>

#include <tuple>
#include <variant>

namespace minire::scene
{
    class Viewpoint
    {
    public:
        bool hasCamera() const { return !std::holds_alternative<std::monostate>(_camera); }

        glm::vec3 const & position() const { return _position; }

        std::tuple<glm::mat4 const &, size_t> mvp() const
        {
            revalidate();
            return std::tie(_mvp, _revision);
        }

    public:
        void setViewport(size_t const width,
                         size_t const height);

        void setTransform(glm::mat4 const & view,
                          glm::vec3 const & position);

        void unsetCamera();

        void setCamera(models::PerspectiveCamera const &);

        void setCamera(models::OrthographicCamera const &);

        size_t width() const { return _vpWidth; }

        size_t height() const { return _vpHeight; }

        glm::mat4 const & view() const { return _view; }

        glm::mat4 const & projection() const { return _projection; }

    private:
        bool isSame(models::PerspectiveCamera const &) const;
        bool isSame(models::OrthographicCamera const &) const;

        void revalidate() const;
        void invalidate();

        void recalcProjection();

    private:
        using Camera = std::variant<std::monostate,
                                    models::PerspectiveCamera,
                                    models::OrthographicCamera>;

        glm::vec3         _position = glm::vec3(0);
        glm::mat4         _view = glm::identity<glm::mat4>();
        glm::mat4         _projection = glm::identity<glm::mat4>();
        Camera            _camera = std::monostate();
        size_t            _vpWidth = 0;
        size_t            _vpHeight = 0;

        mutable size_t    _revision = 0;
        mutable glm::mat4 _mvp = glm::identity<glm::mat4>();
        mutable bool      _invalidated = true;
    };
}
