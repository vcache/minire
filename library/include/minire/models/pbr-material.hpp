#pragma once

#include <minire/content/id.hpp>
#include <minire/models/sampler.hpp>

#include <glm/vec3.hpp>

#include <string>

namespace minire::models
{
    struct PbrMaterial
    {
    public:
        enum class TextureComponent
        {
            kR, kG, kB, kA,
        };

        glm::vec3        _albedoFactor = glm::vec3{1.0f, 1.0f, 1.0f};
        content::MaybeId _albedoTexture = std::nullopt;
        Sampler          _albedoSampler = Sampler{};

        float            _metallicFactor = 1.0f;
        content::MaybeId _metallicTexture = std::nullopt;
        Sampler          _metallicSampler = Sampler{};
        TextureComponent _metallicTextureComponent = TextureComponent::kB;

        float            _roughnessFactor = 1.0f;
        content::MaybeId _roughnessTexture = std::nullopt;
        Sampler          _roughnessSampler = Sampler{};
        TextureComponent _roughnessTextureComponent = TextureComponent::kG;

        content::MaybeId _normalTexture = std::nullopt;
        Sampler          _normalSampler = Sampler{};
        float            _normalScale = 1.0f; // ignored without _normalTexture

        content::MaybeId _aoTexture = std::nullopt;
        Sampler          _aoSampler = Sampler{};
        float            _aoStrength = 1.0f; // or scalar AO if _aoTexture is empty
        TextureComponent _aoTextureComponent = TextureComponent::kR;

        content::MaybeId _emissiveTexture = std::nullopt;
        Sampler          _emissiveSampler = Sampler{};
        glm::vec3        _emissiveFactor = glm::vec3{0.0f, 0.0f, 0.0f};
    };
}
