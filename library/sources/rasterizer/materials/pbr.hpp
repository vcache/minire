#pragma once

#include <minire/material.hpp>
#include <minire/models/pbr-material.hpp>

#include <opengl/program.hpp>
#include <rasterizer/textures.hpp>

#include <glm/vec3.hpp>

#include <optional>
#include <utility>
#include <variant>

namespace minire::rasterizer::materials
{
    class PbrInstance final : public material::Instance
    {
    private:
        explicit PbrInstance(models::PbrMaterial const &,
                             Textures const & textures);

        using UniformData = std::variant<std::monostate,
                                         float,
                                         glm::vec3,
                                         Textures::Texture::Sptr>;

        static UniformData makeUniform(Textures const & textures,
                                       models::TextureMap const & textureMap);

        static GLint setUniform(UniformData const &,
                                opengl::Program const &,
                                GLint location,
                                GLint texUnit);

        UniformData _albedo;
        UniformData _metallic;
        UniformData _roughness;
        UniformData _ao;
        UniformData _normals;

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

        GLint _albedoUniformLocation = -1;
        GLint _metallicUniformLocation = -1;
        GLint _roughnessUniformLocation = -1;
        GLint _aoUniformLocation = -1;
        GLint _normalsUniformLocation = -1;

        GLint _positionAttrLocation = -1;
        GLint _uvAttrLocation = -1;
        GLint _normalAttrLocation = -1;
        GLint _tangentAttrLocation = -1;

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
        static std::string signature(models::TextureMap const & textureMap);

    private:
        Textures const & _textures;
    };
}