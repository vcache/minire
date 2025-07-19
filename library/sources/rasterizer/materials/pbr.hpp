#pragma once

#include <minire/material.hpp>
#include <minire/models/pbr-material.hpp>

#include <opengl/program.hpp>
#include <rasterizer/textures.hpp>

#include <glm/vec3.hpp>

#include <optional>
#include <utility>

namespace minire::rasterizer::materials
{
    class PbrInstance final : public material::Instance
    {
    private:
        explicit PbrInstance(models::PbrMaterial const &,
                             Textures const & textures);

        static void setUniform(float, opengl::Program const &,
                               GLint location);

        static void setUniform(glm::vec3 const &, opengl::Program const &,
                               GLint location);

        static GLint setUniform(Textures::Texture::Sptr const & uniformData,
                                opengl::Program const & glProgram,
                                GLint location, GLint texUnit);

        glm::vec3               _albedoFactor;
        Textures::Texture::Sptr _albedoTexture;

        float                   _metallicFactor;
        Textures::Texture::Sptr _metallicTexture;

        float                   _roughnessFactor;
        Textures::Texture::Sptr _roughnessTexture;

        Textures::Texture::Sptr _normalTexture;
        float                   _normalScale;

        Textures::Texture::Sptr _aoTexture;
        float                   _aoStrength;

        Textures::Texture::Sptr _emissiveTexture;
        glm::vec3               _emissiveFactor;

        friend class PbrProgram;
        friend class PbrFactory;
    };

    class PbrProgram final : public material::Program
    {
    public:
        void prepareDrawing(material::Instance const &,
                            glm::mat4 const & modelTransform,
                            float const colorFactor) const override;

        opengl::Program const & glProgram() const override { return _program; }

        Locations locations() const override;

    public:
        // TODO: make this guy private (but keep compatibility with std::make_shared)
        explicit PbrProgram(opengl::Program && program)
            :  _program(std::move(program))
        {}

    private:
        opengl::Program _program;

        // Uniform locations

        GLint _albedoFactor = -1;
        GLint _albedoTexture = -1;

        GLint _metallicFactor = -1;
        GLint _metallicTexture = -1;

        GLint _roughnessFactor = -1;
        GLint _roughnessTexture = -1;

        GLint _normalTexture = -1;
        GLint _normalScale = -1;

        GLint _aoTexture = -1;
        GLint _aoStrength = -1;

        GLint _emissiveTexture = -1;
        GLint _emissiveFactor = -1;

        GLint _positionAttribute = -1;
        GLint _uvAttribute = -1;
        GLint _normalAttribute = -1;
        GLint _tangentAttribute = -1;

        GLint _modelUniformLocation = -1;
        GLint _colorFactorUniformLocation = -1;

        friend class PbrFactory;
    };

    class PbrFactory final : public material::Factory
    {
    public:
        explicit PbrFactory(Textures const &);

    public:
        material::Program::Sptr build(material::Model const &,
                                      models::MeshFeatures const &) const override;

        material::Instance::Uptr instantiate(material::Model const &,
                                             models::MeshFeatures const &) const override;

        std::string signature(material::Model const &,
                              models::MeshFeatures const &) const override;

    private:
        Textures const & _textures;
    };
}