#pragma once

#include <opengl/fbo.hpp>
#include <opengl/texture.hpp>
#include <rasterizer/culled-objects.hpp>
#include <utils/frustum.hpp>

#include <minire/models/shadow-params.hpp>

#include <glm/vec3.hpp>

#include <cassert>
#include <memory>

namespace minire::utils { class SatPlanes; }

namespace minire::rasterizer
{
    class Materials;

    // Can be used for Point and Spot lights.
    class CubeShadowMap
    {
        CubeShadowMap(CubeShadowMap const &) = delete;
        CubeShadowMap(CubeShadowMap &&) = delete;
        CubeShadowMap & operator=(CubeShadowMap const &) = delete;
        CubeShadowMap & operator=(CubeShadowMap &&) = delete;

    public:
        using Sptr = std::shared_ptr<CubeShadowMap>;
        using CullFunction =
            std::function<rasterizer::CulledPrimitives(utils::SatPlanes const &)>;

        explicit CubeShadowMap(Materials const & materials,
                               models::ShadowParams const &);
        ~CubeShadowMap();

        // Return far plane value
        float perform(glm::vec3 const & lightPosition,
                      utils::ViewFrustum const &,
                      CullFunction cullFunction);

        opengl::Texture const & texture() const
        {
            assert(_shadowTexture || _depthTexture);
            return _shadowTexture ? *_shadowTexture : *_depthTexture;
        }

        models::ShadowParams const & shadowParams() const { return _shadowParams; }

    private:
        class Material;

        Materials const           & _materials;
        models::ShadowParams const _shadowParams;
        size_t const               _meshConsumerKey;
        std::shared_ptr<Material>  _material;
        opengl::Texture::Uptr      _depthTexture;
        opengl::Texture::Uptr      _shadowTexture;
        opengl::FBO                _fbo;
    };
}
