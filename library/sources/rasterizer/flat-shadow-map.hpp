#pragma once

#include <opengl/fbo.hpp>
#include <opengl/program.hpp>
#include <opengl/texture.hpp>
#include <utils/frustum.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <memory>

namespace minire { class Scene; }

namespace minire::rasterizer
{
    // Can be used for directional lights (all rays are parallel).
    class FlatShadowMap
    {
        FlatShadowMap(FlatShadowMap const &) = delete;
        FlatShadowMap(FlatShadowMap &&) = delete;
        FlatShadowMap & operator=(FlatShadowMap const &) = delete;
        FlatShadowMap & operator=(FlatShadowMap &&) = delete;

    public:
        using Sptr = std::shared_ptr<FlatShadowMap>;

        explicit FlatShadowMap(size_t size = 4096);

        // Returns light-space VP-matrix
        glm::mat4 perform(Scene const &,
                          glm::vec3 const & lightPosition,
                          glm::vec3 const & lightDirection,
                          utils::FrustumVertices const &); // TODO: FrustumVertices can be taken directly from Scene (just make it lazy)

        opengl::Texture const & texture() const { return _texture; }

        size_t size() const { return _size; }

    private:
        glm::mat4 buildVP(glm::vec3 const & lightPosition,
                          glm::vec3 const & lightDirection,
                          utils::FrustumVertices const &) const;

    private:
        size_t const    _size;
        opengl::Texture _texture;
        opengl::Program _program;
        opengl::FBO     _fbo;
        GLint           _bznkLightMatrix = 0;
        GLint           _bznkModelMatrix = 0;
    };
}
