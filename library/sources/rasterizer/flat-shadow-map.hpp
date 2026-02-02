#pragma once

#include <opengl/fbo.hpp>
#include <opengl/texture.hpp>
#include <rasterizer/culled-objects.hpp>
#include <utils/frustum.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <memory>

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
        ~FlatShadowMap();

        // Returns light-space VP-matrix
        glm::mat4 perform(CulledPrimitives const &,
                          glm::vec3 const & lightPosition,
                          glm::vec3 const & lightDirection,
                          utils::FrustumVertices const &);

        opengl::Texture const & texture() const { return _texture; }

        size_t size() const { return _size; }

    private:
        glm::mat4 buildVP(glm::vec3 const & lightPosition,
                          glm::vec3 const & lightDirection,
                          utils::FrustumVertices const &) const;

    private:
        class Factory;
        using FactoryUptr = std::unique_ptr<Factory>;

        size_t const    _size;
        FactoryUptr     _factory;
        opengl::Texture _texture;
        opengl::FBO     _fbo;
    };
}
