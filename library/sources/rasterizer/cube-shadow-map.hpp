#pragma once

#include <opengl/fbo.hpp>
#include <opengl/texture.hpp>
#include <rasterizer/culled-objects.hpp>
#include <utils/frustum.hpp>

#include <glm/vec3.hpp>

#include <memory>

namespace minire::rasterizer
{
    // Can be used for Point and Spot lights.
    class CubeShadowMap
    {
        CubeShadowMap(CubeShadowMap const &) = delete;
        CubeShadowMap(CubeShadowMap &&) = delete;
        CubeShadowMap & operator=(CubeShadowMap const &) = delete;
        CubeShadowMap & operator=(CubeShadowMap &&) = delete;

    public:
        using Sptr = std::shared_ptr<CubeShadowMap>;

        explicit CubeShadowMap(size_t size = 1024);

        ~CubeShadowMap();

        // Return far plane value
        float perform(CulledPrimitives const &,
                      glm::vec3 const & lightPosition,
                      utils::FrustumVertices const &);

        opengl::Texture const & texture() const { return _texture; }

        size_t size() const { return _size; }

    private:
        class Factory;
        using FactoryUptr = std::unique_ptr<Factory>;

        size_t const    _size;
        FactoryUptr     _factory;
        opengl::Texture _texture;
        opengl::FBO     _fbo;
        GLint           _bznkModelMatrix = 0;
        GLint           _bznkShadowMatrices = 0;
        GLint           _bznkLightPos = 0;
        GLint           _bznkFarPlane = 0;
    };
}