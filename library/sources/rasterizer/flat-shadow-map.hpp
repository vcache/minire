#pragma once

#include <opengl/fbo.hpp>
#include <opengl/texture.hpp>
#include <rasterizer/culled-objects.hpp>
#include <utils/frustum.hpp>

#include <minire/models/shadow-params.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cassert>
#include <memory>

namespace minire::rasterizer::filters { class GaussianBlur; }

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

        explicit FlatShadowMap(models::ShadowParams const &);
        ~FlatShadowMap();

        // Returns light-space VP-matrix
        glm::mat4 perform(CulledPrimitives const &,
                          glm::vec3 const & lightPosition,
                          glm::vec3 const & lightDirection,
                          utils::ViewFrustum const &);

        opengl::Texture const & texture() const
        {
            assert(_shadowTexture || _depthTexture);
            return _shadowTexture ? *_shadowTexture : *_depthTexture;
        }

        models::ShadowParams const & shadowParams() const { return _shadowParams; }

    private:
        glm::mat4 buildVP(glm::vec3 const & lightPosition,
                          glm::vec3 const & lightDirection,
                          utils::ViewFrustum const &) const;

    private:
        class Factory;
        using FactoryUptr = std::unique_ptr<Factory>;
        using GaussianBlurUptr = std::unique_ptr<filters::GaussianBlur>;

        models::ShadowParams const _shadowParams;
        FactoryUptr                _factory;
        opengl::Texture::Uptr      _depthTexture;
        opengl::Texture::Uptr      _shadowTexture;
        opengl::FBO                _fbo;
        GaussianBlurUptr           _gaussianBlur;
    };
}
