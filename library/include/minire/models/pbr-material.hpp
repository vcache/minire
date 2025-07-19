#pragma once

#include <minire/content/id.hpp>
#include <minire/material.hpp>
#include <minire/models/sampler.hpp>

#include <glm/vec3.hpp>

#include <string>

namespace minire::models
{
    class PbrMaterial : public material::Model
    {
    public:
        enum class TextureComponent
        {
            kR, kG, kB, kA,
        };

        glm::vec3        _albedoFactor = glm::vec3{1.0f, 1.0f, 1.0f};
        content::MaybeId _albedoTexture;
        Sampler          _albedoSampler;

        float            _metallicFactor = 1.0f;
        content::MaybeId _metallicTexture;
        Sampler          _metallicSampler;
        TextureComponent _metallicTextureComponent = TextureComponent::kB;

        float            _roughnessFactor = 1.0f;
        content::MaybeId _roughnessTexture;
        Sampler          _roughnessSampler;
        TextureComponent _roughnessTextureComonent = TextureComponent::kG;

        content::MaybeId _normalTexture;
        Sampler          _normalSampler;
        float            _normalScale = 1.0f; // ignored without _normalTexture

        content::MaybeId _aoTexture;
        Sampler          _aoSampler;
        float            _aoStrength = 1.0f; // or scalar AO if _aoTexture is empty
        TextureComponent _aoTextureComonent = TextureComponent::kR;

        content::MaybeId _emissiveTexture;
        Sampler          _emissiveSampler;
        glm::vec3        _emissiveFactor = glm::vec3{0.0f, 0.0f, 0.0f};

    public:
        std::string const & materialKind() const override
        {
            return kMaterialKind;
        }

    public:
        static std::string const kMaterialKind;
    };
}
