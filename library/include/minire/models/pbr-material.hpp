#pragma once

#include <minire/material.hpp>
#include <minire/models/texture-map.hpp>

#include <string>

namespace minire::models
{
    class PbrMaterial : public material::Model
    {
    public:
        TextureMap _albedo;
        TextureMap _metallic;
        TextureMap _roughness;
        TextureMap _ao;
        TextureMap _normals;

    public:
        PbrMaterial(TextureMap albedo,
                    TextureMap metallic,
                    TextureMap roughness,
                    TextureMap ao,
                    TextureMap normals)
            : _albedo(std::move(albedo))
            , _metallic(std::move(metallic))
            , _roughness(std::move(roughness))
            , _ao(std::move(ao))
            , _normals(std::move(normals))
        {}

    public:
        std::string const & materialKind() const override
        {
            return kMaterialKind;
        }

    public:
        static std::string const kMaterialKind;
    };
}
