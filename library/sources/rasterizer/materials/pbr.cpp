#include <rasterizer/materials/pbr.hpp>

#include <minire/errors.hpp>
#include <minire/models/mesh-features.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <rasterizer/constants.hpp>
#include <rasterizer/ubo.hpp>

#include <inja/inja.hpp>
#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr

#include <fmt/format.h>

#include <cassert>

namespace minire::rasterizer::materials
{
    namespace
    {
        std::string toString(models::PbrMaterial::TextureComponent textureComponent)
        {
            switch(textureComponent)
            {
                case models::PbrMaterial::TextureComponent::kR: return "r";
                case models::PbrMaterial::TextureComponent::kG: return "g";
                case models::PbrMaterial::TextureComponent::kB: return "b";
                case models::PbrMaterial::TextureComponent::kA: return "a";
            }
            MINIRE_THROW("unexpected texture component: {}",
                         static_cast<int>(textureComponent));
        }
    }

    // PbrInstance //

    PbrInstance::PbrInstance(models::PbrMaterial const & pbrModel,
                             Textures const & textures)
        : _albedoFactor(pbrModel._albedoFactor)
        , _albedoTexture(textures.get(pbrModel._albedoTexture, pbrModel._albedoSampler))
        , _metallicFactor(pbrModel._metallicFactor)
        , _metallicTexture(textures.get(pbrModel._metallicTexture, pbrModel._metallicSampler))
        , _roughnessFactor(pbrModel._roughnessFactor)
        , _roughnessTexture(textures.get(pbrModel._roughnessTexture, pbrModel._roughnessSampler))
        , _normalTexture(textures.get(pbrModel._normalTexture, pbrModel._normalSampler))
        , _normalScale(pbrModel._normalScale)
        , _aoTexture(textures.get(pbrModel._aoTexture, pbrModel._aoSampler))
        , _aoStrength(pbrModel._aoStrength)
        , _emissiveTexture(textures.get(pbrModel._emissiveTexture, pbrModel._emissiveSampler))
        , _emissiveFactor(pbrModel._emissiveFactor)
    {}

    void PbrInstance::setUniform(float value, opengl::Program const & glProgram,
                                 GLint location)
    {
        assert(location != -1);
        glProgram.setUniform(location, value);
    }

    void PbrInstance::setUniform(glm::vec3 const & value,
                                 opengl::Program const & glProgram,
                                 GLint location)
    {
        assert(location != -1);
        glProgram.setUniform(location, value);
    }

    GLint PbrInstance::setUniform(Textures::Texture::Sptr const & texture,
                                  opengl::Program const & glProgram,
                                  GLint location, GLint texUnit)
    {
        if (!texture) return 0;

        assert(location != -1);
        MINIRE_GL(glActiveTexture, GL_TEXTURE0 + texUnit);
        glProgram.setUniform(location, texUnit);
        texture->bind();
        return 1;
    }

    // PbrProgram //

    void PbrProgram::prepareDrawing(material::Instance const & instance,
                                    glm::mat4 const & modelTransform,
                                    float const colorFactor) const
    {
        _program.use();

        // setup global states

        _program.setUniform(_modelUniformLocation, modelTransform);
        _program.setUniform(_colorFactorUniformLocation, colorFactor);

        // setup mappers

        // TODO: can it be a static_cast?
        auto pbrInstance = dynamic_cast<PbrInstance const &>(instance);

        GLint texUnit = 0;

        PbrInstance::setUniform(pbrInstance._albedoFactor, _program, _albedoFactor);
        texUnit += PbrInstance::setUniform(pbrInstance._albedoTexture, _program, _albedoTexture, texUnit);

        PbrInstance::setUniform(pbrInstance._metallicFactor, _program, _metallicFactor);
        texUnit += PbrInstance::setUniform(pbrInstance._metallicTexture, _program, _metallicTexture, texUnit);

        PbrInstance::setUniform(pbrInstance._roughnessFactor, _program, _roughnessFactor);
        texUnit += PbrInstance::setUniform(pbrInstance._roughnessTexture, _program, _roughnessTexture, texUnit);

        if (pbrInstance._normalTexture)
        {
            texUnit += PbrInstance::setUniform(pbrInstance._normalTexture, _program, _normalTexture, texUnit);
            PbrInstance::setUniform(pbrInstance._normalScale, _program, _normalScale);
        }

        texUnit += PbrInstance::setUniform(pbrInstance._aoTexture, _program, _aoTexture, texUnit);
        PbrInstance::setUniform(pbrInstance._aoStrength, _program, _aoStrength);

        texUnit += PbrInstance::setUniform(pbrInstance._emissiveTexture, _program, _emissiveTexture, texUnit);
        PbrInstance::setUniform(pbrInstance._emissiveFactor, _program, _emissiveFactor);
    }

    material::Program::Locations PbrProgram::locations() const
    {
        return material::Program::Locations
        {
            ._vertexAttribute = _positionAttribute,
            ._uvAttribute = _uvAttribute,
            ._normalAttribute = _normalAttribute,
            ._tangentAttribute = _tangentAttribute,
        };
    }

    // PbrFactory //

    PbrFactory::PbrFactory(Textures const & textures)
        : _textures(textures)
    {}

    material::Program::Sptr PbrFactory::build(material::Model const & model,
                                              models::MeshFeatures const & features) const
    {
        // TODO: maybe it could be static cast?
        auto pbrModel = dynamic_cast<models::PbrMaterial const &>(model);

        // Prepare template parameters

        nlohmann::json vars
        {
            {"kHasUvs",              features.hasUv()},
            {"kHasNormals",          features.hasNormal()},
            {"kHasTangents",         features.hasTangent()},

            {"kHasAlbedoTexture",    pbrModel._albedoTexture.has_value()},
            {"kHasMetallicTexture",  pbrModel._metallicTexture.has_value()},
            {"kHasRoughnessTexture", pbrModel._roughnessTexture.has_value()},
            {"kHasNormalTexture",    pbrModel._normalTexture.has_value()},
            {"kHasAoTexture",        pbrModel._aoTexture.has_value()},
            {"kHasEmissiveTexture",  pbrModel._emissiveTexture.has_value()},

            {"kMetallicTexComp",     toString(pbrModel._metallicTextureComponent)},
            {"kRoughnessTexComp",    toString(pbrModel._roughnessTextureComonent)},
            {"kAoTexComp",           toString(pbrModel._aoTextureComonent)},

            {"kUboDatablock",        Ubo::interfaceBlock()},
        };

        // Init template render

        inja::Environment env;
        env.include_template("shaders/pbr-kit.incl",
                             env.parse(Constants::kPbrKit));

        // Render the shaders

        std::string vertShader = env.render(Constants::kPbrVertShader, vars);
        std::string fragShader = env.render(Constants::kPbrFragShader, vars);

        // Make shaders and build a program

        opengl::Program program(
        {
            std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, vertShader),
            std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, fragShader),
        });

        // Collect uniforms, attribs locations and build the result

        auto result = std::make_shared<PbrProgram>(std::move(program));

        result->_albedoFactor = result->_program.getUniformLocation("bznkAlbedoFactor");
        result->_albedoTexture = result->_program.getUniformLocation("bznkAlbedoTexture");

        result->_metallicFactor = result->_program.getUniformLocation("bznkMetallicFactor");
        result->_metallicTexture = result->_program.getUniformLocation("bznkMetallicTexture");

        result->_roughnessFactor = result->_program.getUniformLocation("bznkRoughnessFactor");
        result->_roughnessTexture = result->_program.getUniformLocation("bznkRoughnessTexture");

        result->_normalTexture = result->_program.getUniformLocation("bznkNormalTexture");
        result->_normalScale = result->_program.getUniformLocation("bznkNormalScale");

        result->_aoTexture = result->_program.getUniformLocation("bznkAoTexture");
        result->_aoStrength = result->_program.getUniformLocation("bznkAoStrength");

        result->_emissiveTexture = result->_program.getUniformLocation("bznkEmissiveTexture");
        result->_emissiveFactor = result->_program.getUniformLocation("bznkEmissiveFactor");

        result->_modelUniformLocation = result->_program.getUniformLocation("bznkModel");
        assert(result->_modelUniformLocation != -1);

        result->_colorFactorUniformLocation = result->_program.getUniformLocation("bznkColorFactor");
        assert(result->_colorFactorUniformLocation != -1);

        result->_positionAttribute = result->_program.getAttribLocation("bznkVertex");
        assert(result->_positionAttribute != -1);

        result->_uvAttribute = features.hasUv() ? result->_program.getAttribLocation("bznkUv") : -1;
        result->_normalAttribute = features.hasNormal() ? result->_program.getAttribLocation("bznkNormal") : -1;
        result->_tangentAttribute = features.hasTangent() ? result->_program.getAttribLocation("bznkTangent") : -1;

        return result;
    }

    material::Instance::Uptr PbrFactory::instantiate(material::Model const & model,
                                                     models::MeshFeatures const &) const
    {
        // TODO: maybe it could be static cast?
        auto pbrModel = dynamic_cast<models::PbrMaterial const &>(model);

        // NOTE: have to call "operator new()" due to private ctor
        return std::unique_ptr<PbrInstance>(new PbrInstance(pbrModel, _textures));
    }

    std::string PbrFactory::signature(material::Model const & model,
                                      models::MeshFeatures const & features) const
    {
        // TODO: maybe it could be static cast?
        auto pbrModel = dynamic_cast<models::PbrMaterial const &>(model);

        std::string result = models::PbrMaterial::kMaterialKind + "/";

        result += features.hasUv() ? "UV/" : "";
        result += features.hasNormal() ? "N/" : "";
        result += features.hasTangent() ? "T/" : "";

        result += pbrModel._albedoTexture ? "A:TF/" : "A:F/";
        result += pbrModel._metallicTexture ? fmt::format("M:T{}F/", static_cast<int>(pbrModel._metallicTextureComponent))
                                            : std::string("M:F/");
        result += pbrModel._roughnessTexture ? fmt::format("R:T{}F/", static_cast<int>(pbrModel._roughnessTextureComonent))
                                             : std::string("R:F/");
        result += pbrModel._normalTexture ? "N:T/" : "N:0/";
        result += pbrModel._aoTexture ? fmt::format("O:T{}/", static_cast<int>(pbrModel._aoTextureComonent))
                                      : std::string("O:0/");
        result += pbrModel._emissiveTexture ? "E:T/" : "E:0/";

        result += "!";
        return result;
    }
}
