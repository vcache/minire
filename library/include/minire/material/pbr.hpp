#pragma once

#include <minire/material.hpp>
#include <minire/models/pbr-material.hpp>

#include <limits>
#include <memory>
#include <optional>

namespace minire::material
{
    class Pbr
        : public Material
    {
    public:
        using Sptr = std::shared_ptr<Pbr>;
        using Wptr = std::weak_ptr<Pbr>;

        explicit Pbr(models::PbrMaterial const &);

        material::Program render() const override;
        material::UniformValues const & updateUserUniforms(material::UniformNames const &,
                                                           models::TextureResolver const &) const override;

    protected:
        std::string slugImpl() const override;

    protected:
        models::PbrMaterial const       _model;

    private:
        struct UniformIndeces
        {
            static constexpr size_t kNoIndex = std::numeric_limits<size_t>::max();

            size_t _albedoFactor     = kNoIndex;
            size_t _albedoTexture    = kNoIndex;
            size_t _metallicFactor   = kNoIndex;
            size_t _metallicTexture  = kNoIndex;
            size_t _roughnessFactor  = kNoIndex;
            size_t _roughnessTexture = kNoIndex;
            size_t _normalTexture    = kNoIndex;
            size_t _normalScale      = kNoIndex;
            size_t _aoStrength       = kNoIndex;
            size_t _aoTexture        = kNoIndex;
            size_t _emissiveTexture  = kNoIndex;

            explicit UniformIndeces(material::UniformNames const &);
        };

        mutable std::optional<UniformIndeces> _indeces;
        mutable material::UniformValues       _values;
        mutable bool                          _modelInvalidated;

        mutable models::TextureHandle::Sptr   _albedoTextureHandle;
        mutable models::TextureHandle::Sptr   _metallicTextureHandle;
        mutable models::TextureHandle::Sptr   _roughnessTextureHandle;
        mutable models::TextureHandle::Sptr   _normalTextureHandle;
        mutable models::TextureHandle::Sptr   _aoTextureHandle;
        mutable models::TextureHandle::Sptr   _emissiveTextureHandle;
    };
}
