#include <rasterizer/materials/pbr.hpp>

#include <minire/errors.hpp>
#include <minire/models/mesh-features.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <rasterizer/constants.hpp>
#include <rasterizer/ubo.hpp>
#include <utils/overloaded.hpp>

#include <inja/inja.hpp>
#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr

#include <fmt/format.h>

#include <cassert>

namespace minire::rasterizer::materials
{
    static const std::string kMapperNone = "none";
    static const std::string kMapperFloat = "float";
    static const std::string kMapperVec3 = "vec3";
    static const std::string kMapperTex2D = "tex2D";

    // PbrInstance //

    PbrInstance::UniformData PbrInstance::makeUniform(Textures const & textures,
                                                      models::TextureMap const & textureMap)
    {
        return std::visit(utils::Overloaded
        {
            [](std::monostate) -> UniformData       { return std::monostate(); },
            [](float v) -> UniformData              { return v; },
            [](glm::vec3 const & v) -> UniformData  { return v; },
            [&textures](content::Id const & v) -> UniformData
            {
                auto texture = textures.get(v);
                MINIRE_INVARIANT(texture, "texture not provided");
                return texture;
            },
        }, textureMap);
    }

    PbrInstance::PbrInstance(models::PbrMaterial const & pbrModel,
                             Textures const & textures)
        : _albedo(makeUniform(textures,     pbrModel._albedo))
        , _metallic(makeUniform(textures,   pbrModel._metallic))
        , _roughness(makeUniform(textures,  pbrModel._roughness))
        , _ao(makeUniform(textures,         pbrModel._ao))
        , _normals(makeUniform(textures,    pbrModel._normals))
    {}

    // Returns amount of texUnits has been used
    GLint PbrInstance::setUniform(UniformData const & uniformData,
                                 opengl::Program const & glProgram,
                                 GLint location,
                                 GLint texUnit)
    {
        return std::visit(utils::Overloaded
        {
            [](std::monostate)       { return 0; },
            [&](float v)             { glProgram.setUniform(location, v); return 0; },
            [&](glm::vec3 const & v) { glProgram.setUniform(location, v); return 0; },
            [&](Textures::Texture::Sptr const & texture)
            {
                assert(texture);
                MINIRE_GL(glActiveTexture, GL_TEXTURE0 + texUnit);
                glProgram.setUniform(location, texUnit);
                texture->bind();
                return 1;
            },
        }, uniformData);
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
        texUnit += PbrInstance::setUniform(pbrInstance._albedo, _program, _albedoUniformLocation, texUnit);
        texUnit += PbrInstance::setUniform(pbrInstance._metallic, _program, _metallicUniformLocation, texUnit);
        texUnit += PbrInstance::setUniform(pbrInstance._roughness, _program, _roughnessUniformLocation, texUnit);
        texUnit += PbrInstance::setUniform(pbrInstance._ao, _program, _aoUniformLocation, texUnit);
        texUnit += PbrInstance::setUniform(pbrInstance._normals, _program, _normalsUniformLocation, texUnit);
    }

    material::Program::Locations PbrProgram::locations() const
    {
        return material::Program::Locations
        {
            ._vertexAttribute = _positionAttrLocation,
            ._uvAttribute = _uvAttrLocation,
            ._normalAttribute = _normalAttrLocation,
            ._tangentAttribute = _tangentAttrLocation,
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

        MINIRE_INVARIANT(!std::holds_alternative<std::monostate>(pbrModel._albedo), "albedo required");
        MINIRE_INVARIANT(!std::holds_alternative<float>(pbrModel._albedo), "albedo cannot be a float");
        MINIRE_INVARIANT(!std::holds_alternative<float>(pbrModel._normals), "normals cannot be a float");
        MINIRE_INVARIANT(!std::holds_alternative<glm::vec3>(pbrModel._normals), "normals cannot be a vec3");

        // Prepare template parameters

        nlohmann::json vars
        {
            {"kHasUvs",              features.hasUv()},
            {"kHasNormals",          features.hasNormal()},
            {"kHasTangents",         features.hasTangent()},

            {"kHasAlbedoUniform",    !std::holds_alternative<std::monostate>(pbrModel._albedo)},
            {"kHasMetallicUniform",  !std::holds_alternative<std::monostate>(pbrModel._metallic)},
            {"kHasRoughnessUniform", !std::holds_alternative<std::monostate>(pbrModel._roughness)},
            {"kHasAoUniform",        !std::holds_alternative<std::monostate>(pbrModel._ao)},
            {"kHasNormalsUniform",   !std::holds_alternative<std::monostate>(pbrModel._normals)},

            {"kAlbedoUniformType",   signature(pbrModel._albedo)},
            {"kMetallicUniformType", signature(pbrModel._metallic)},
            {"kRoughnessUniformType",signature(pbrModel._roughness)},
            {"kAoUniformType",       signature(pbrModel._ao)},
            {"kNormalsUniformType",  signature(pbrModel._normals)},

            {"kUboDatablock",        Ubo::interfaceBlock()},
        };

        // Init template render

        inja::Environment env;
        env.include_template("shaders/pbr-kit.incl",
                             env.parse(Constants::kPbrKit));

        // Register helper functions

        env.add_callback("mkUniformDef", 2, [](inja::Arguments const & args)
            {
                std::string const & code = args.at(0)->get<std::string>();
                std::string const & id = args.at(1)->get<std::string>();

                if (code == kMapperNone)
                {
                    return std::string();
                }
                else if (code == kMapperFloat)
                {
                    return fmt::format("uniform float {};", id);
                }
                else if (code == kMapperVec3)
                {
                    return fmt::format("uniform vec3 {};", id);
                }
                else if (code == kMapperTex2D)
                {
                    return fmt::format("uniform sampler2D {};", id);
                }

                MINIRE_THROW("bad uniform kind code: {} for {}", code, id);
            });

        env.add_callback("mkValueSampler", 3, [](inja::Arguments const & args)
            {
                std::string const & code = args.at(0)->get<std::string>();
                std::string const & id = args.at(1)->get<std::string>();
                size_t const texMod = args.at(2)->get<size_t>();

                if (code == kMapperNone)
                {
                    return std::string(texMod == 2 ? "vec3(0)" : "0.0");
                }
                else if (code == kMapperFloat)
                {
                    return id;
                }
                else if (code == kMapperVec3)
                {
                    return id;
                }
                else if (code == kMapperTex2D)
                {
                    if (texMod == 0)
                    {
                        return fmt::format("pow(texture({}, bznkFragUv).rgb, vec3(2.2))", id);
                    }
                    else if (texMod == 1)
                    {
                        return fmt::format("texture({}, bznkFragUv).r", id);
                    }
                    else if (texMod == 2)
                    {
                        return fmt::format("texture({}, bznkFragUv).rgb", id);
                    }
                }

                MINIRE_THROW("bad uniform kind: code = {}, id = {}, texMod = {}",
                             code, id, texMod);
            });

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

        auto loadUniformLoc = [&result](models::TextureMap const & map,
                                        char const * name,
                                        GLint & output)
        {
            if (!std::holds_alternative<std::monostate>(map))
            {
                output = result->_program.getUniformLocation(name);
                assert(output != -1);
            }
        };

        loadUniformLoc(pbrModel._albedo, "bznkAlbedo", result->_albedoUniformLocation);
        loadUniformLoc(pbrModel._metallic, "bznkMetallic", result->_metallicUniformLocation);
        loadUniformLoc(pbrModel._roughness, "bznkRoughness", result->_roughnessUniformLocation);
        loadUniformLoc(pbrModel._ao, "bznkAo", result->_aoUniformLocation);
        loadUniformLoc(pbrModel._normals, "bznkNormals", result->_normalsUniformLocation);

        result->_modelUniformLocation = result->_program.getUniformLocation("bznkModel");
        assert(result->_modelUniformLocation != -1);

        result->_colorFactorUniformLocation = result->_program.getUniformLocation("bznkColorFactor");
        assert(result->_colorFactorUniformLocation != -1);

        result->_positionAttrLocation = result->_program.getAttribLocation("bznkVertex");
        assert(result->_positionAttrLocation != -1);

        result->_uvAttrLocation = features.hasUv() ? result->_program.getAttribLocation("bznkUv") : -1;
        result->_normalAttrLocation = features.hasNormal() ? result->_program.getAttribLocation("bznkNormal") : -1;
        result->_tangentAttrLocation = features.hasTangent() ? result->_program.getAttribLocation("bznkTangent") : -1;

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

    std::string PbrFactory::signature(models::TextureMap const & textureMap)
    {
        return std::visit(utils::Overloaded
        {
            [](std::monostate)      { return kMapperNone; },
            [](float)               { return kMapperFloat; },
            [](glm::vec3 const &)   { return kMapperVec3; },
            [](content::Id const &) { return kMapperTex2D; },
        }, textureMap);
    }

    std::string PbrFactory::signature(material::Model const & model,
                                      models::MeshFeatures const & features) const
    {
        // TODO: maybe it could be static cast?
        auto pbrModel = dynamic_cast<models::PbrMaterial const &>(model);
        return fmt::format("{}/f:{}{}{}/a:{}/m:{}/r:{}/ao:{}/n:{}",
                           models::PbrMaterial::kMaterialKind,
                           features.hasUv(),
                           features.hasNormal(),
                           features.hasTangent(),
                           signature(pbrModel._albedo),
                           signature(pbrModel._metallic),
                           signature(pbrModel._roughness),
                           signature(pbrModel._ao),
                           signature(pbrModel._normals));
    }
}
