#include <rasterizer/materials.hpp>

#include <minire/content/id.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/overloaded.hpp>

#include <opengl/program.hpp>
#include <rasterizer/constants.hpp>
#include <rasterizer/materials/templates.hpp>
#include <rasterizer/ubo.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>
#include <inja/inja.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <type_traits>
#include <unordered_set>

namespace minire::rasterizer
{
    namespace
    {
        struct BuiltinUniformNames
        {
            std::optional<std::string> _model;
            std::optional<std::string> _bones;
            std::optional<std::string> _directionalLightsShadowMaps;
            std::optional<std::string> _pointLightsShadowMaps;
            std::optional<std::string> _emissiveFactor;
            std::optional<std::string> _ambientLight;
            std::optional<std::string> _meshId;
        };

        struct AttribNames
        {
            std::optional<std::string>  _vertex;
            std::optional<std::string>  _uv;
            std::optional<std::string>  _normal;
            std::optional<std::string>  _tangent;
            std::optional<std::string>  _joints;
            std::optional<std::string>  _weights;
        };

        GLenum toGlEnum(material::ShaderType shaderType)
        {
            switch(shaderType)
            {
                case material::ShaderType::kVertex:         return GL_VERTEX_SHADER;
                case material::ShaderType::kFragment:       return GL_FRAGMENT_SHADER;
                case material::ShaderType::kGeometry:       return GL_GEOMETRY_SHADER;
                case material::ShaderType::kTessControl:    return GL_TESS_CONTROL_SHADER;
                case material::ShaderType::kTessEvaluation: return GL_TESS_EVALUATION_SHADER;        
                case material::ShaderType::kCompute:        return GL_COMPUTE_SHADER;
                case material::ShaderType::__kCount__:      MINIRE_THROW("__kCount__ is illegal shader type");
            }
            MINIRE_THROW("bad enum type: {}", static_cast<int>(shaderType));
        }

        void setSetOnce(inja::Arguments const & args,
                        std::optional<std::string> & output)
        {
            assert(args.size() == 1);
            std::string const & value = args.at(0)->get<std::string>();
            MINIRE_INVARIANT(!output || *output == value,
                             "cannot modify value that is already set: {} -> {}", output, value);
            output = value;
        }

        std::vector<opengl::Shader::Sptr> makeShaders(material::Shaders const & shaders,
                                                      nlohmann::json const & templateParams) try
        {
            std::vector<opengl::Shader::Sptr> result;
            result.reserve(shaders.size());
            for(size_t i = 0; i < static_cast<size_t>(material::ShaderType::__kCount__); ++i)
            {
                if (!shaders[i].empty())
                {
                    result.emplace_back(std::make_shared<opengl::Shader>(
                        toGlEnum(static_cast<material::ShaderType>(i)), shaders[i]));
                }
            }
            return result;
        }
        catch(std::exception const & e)
        {
            MINIRE_THROW("shader set compilation failed: {}\n\n"
                         "Shaders set sources:\n\n{}\n\nGeneric params:\n{}",
                         e.what(), fmt::join(shaders, "\n=============================\n"), templateParams.dump(4));
        }
        catch(...)
        {
            MINIRE_THROW("shader set compilation failed (unknown exception)\n\n"
                        "Shaders set sources:\n\n{}\n\nGeneric params:\n{}",
                         fmt::join(shaders, "\n=============================\n"), templateParams.dump(4));
        }
    }

    // Materials::Brush::TextureUnitHelper //

    class Materials::Brush::TextureUnitHelper
    {
    public:
        explicit TextureUnitHelper(opengl::Program const & program,
                                   GLint texUnit = 0)
            : _program(program)
            , _texUnit(texUnit)
        {}

        void setTextureUniform(GLint location, Textures::Texture & texture)
        {
            assert(location != -1);
            GLint texUnit = activateNextUnit();
            _program.setUniform(location, texUnit);
            texture.bind();
        }

        GLint activateNextUnit()
        {
            MINIRE_INVARIANT(_texUnit < maxUnits(),
                             "rendering required {} texture units, while only {} avalilable",
                             _texUnit, maxUnits());
            MINIRE_GL(glActiveTexture, GL_TEXTURE0 + _texUnit);
            return _texUnit++;
        }

    private:
        static GLint maxUnits()
        {
            static const GLint kMaxUnits = []()
            {
                GLint units = 0;
                MINIRE_GL(glGetIntegerv, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &units);
                return units;
            }();
            return kMaxUnits;
        }

    private:
        opengl::Program const & _program;
        GLint                   _texUnit = 0;
    };

    // Materials::TemplateRenderingOutput //

    struct Materials::TemplateRenderingOutput
    {
        AttribNames                     _attribNames;
        std::optional<std::string>      _uboName;
        BuiltinUniformNames             _builtinUniforms;
        std::unordered_set<std::string> _userUniforms;
        std::unordered_set<std::string> _activeGuards;
    };

    // Materials::Brush //

    Materials::Brush::Brush(Textures const & textures,
                            Ubo const & ubo,
                            Materials::TemplateRenderingOutput const & tro,
                            material::Shaders && sources,
                            std::unique_ptr<nlohmann::json> && templateParams,
                            models::MeshFeatures const & meshFeatures,
                            Material::Sptr const & material)
        : _textures(textures)
        , _sources(std::move(sources))
        , _templateParams(std::move(templateParams))
        , _meshFeatures(meshFeatures)
        , _material(material)
        , _program(makeShaders(_sources, *_templateParams))
        , _attribLocations(_program.getAttribLocation(tro._attribNames._vertex),
                           _program.getAttribLocation(tro._attribNames._uv),
                           _program.getAttribLocation(tro._attribNames._normal),
                           _program.getAttribLocation(tro._attribNames._tangent),
                           _program.getAttribLocation(tro._attribNames._joints),
                           _program.getAttribLocation(tro._attribNames._weights))
        , _modelUniform(_program.getUniformLocation(tro._builtinUniforms._model))
        , _bonesUniform(_program.getUniformLocation(tro._builtinUniforms._bones))
        , _directionalLightsShadowMapsUniform(_program.getUniformLocation(tro._builtinUniforms._directionalLightsShadowMaps))
        , _pointLightsShadowMapsUniform(_program.getUniformLocation(tro._builtinUniforms._pointLightsShadowMaps))
        , _emissiveFactorUniform(_program.getUniformLocation(tro._builtinUniforms._emissiveFactor))
        , _ambientLightUniform(_program.getUniformLocation(tro._builtinUniforms._ambientLight))
        , _meshIdUniform(_program.getUniformLocation(tro._builtinUniforms._meshId))
        , _userUniforms(tro._userUniforms.cbegin(), tro._userUniforms.cend())
    {
        assert(_templateParams); // a bit late, but still
        assert(_material);

        // setup UBO
        if (tro._uboName)
        {
            _program.use();
            ubo.bindBufferRange(_program, *tro._uboName);
        }

        // pre-process user uniforms
        _userUniformMeta.reserve(_userUniforms.size());
        for(material::UserUniform const & userUniform : _userUniforms)
        {
            GLint const location = _program.getUniformLocation(userUniform.name());
            MINIRE_INVARIANT(location >= 0, "failed to resolve uniform location: \"{}\"",
                             userUniform.name());
            _userUniformMeta.emplace_back(UserUniformMeta
            {
                ._texture = {},
                ._contentId = {},
                ._location = location,
            });
        }
    }

    void Materials::Brush::setupBuiltinUniforms(TextureUnitHelper & textureUnitHelper,
                                                glm::mat4 const & modelTransform,
                                                glm::vec3 const & ambientLight,
                                                glm::vec3 const & emissiveFactor,
                                                material::TextureRefs const & directionalLightsShadowMaps,
                                                material::TextureRefs const & pointLightsShadowMaps,
                                                material::SkinningVector const & skinningVector,
                                                uint32_t const meshId) const
    {
        MINIRE_INVARIANT(skinningVector.empty() || (_bonesUniform >= 0 &&
                                                    _attribLocations.jointsAttribute() >= 0),
                         "mesh has skinningVector while mesh's program doesn't support skinning");

        if (_modelUniform != -1)
        {
            assert(skinningVector.empty());
            assert(_bonesUniform == -1);
            _program.setUniform(_modelUniform, modelTransform);
        }
        else
        {
            assert(!skinningVector.empty());
        }

        if (_ambientLightUniform != -1) _program.setUniform(_ambientLightUniform, ambientLight);
        if (_emissiveFactorUniform != -1) _program.setUniform(_emissiveFactorUniform, emissiveFactor);

        // directional lights shadow maps
        if (_directionalLightsShadowMapsUniform != -1)
        {
            MINIRE_INVARIANT(directionalLightsShadowMaps.size() <= Ubo::maxDirectionalLights(),
                             "provided {} shadow maps for directional lights, while limit is {}",
                             directionalLightsShadowMaps.size(), Ubo::maxDirectionalLights());
            std::array<GLint, Ubo::maxDirectionalLights()> directionalLightsSamplers;
            for(size_t i = 0; i < directionalLightsSamplers.size(); ++i)
            {
                directionalLightsSamplers[i] = textureUnitHelper.activateNextUnit();
                if (i < directionalLightsShadowMaps.size() &&
                    directionalLightsShadowMaps[i])
                {
                    directionalLightsShadowMaps[i]->bind();
                }
                else
                {
                    MINIRE_GL(glBindTexture, GL_TEXTURE_2D, 0);
                }
            }
            _program.setUniform(_directionalLightsShadowMapsUniform, directionalLightsSamplers);
        }

        // point lights shadow maps
        if (_pointLightsShadowMapsUniform != -1)
        {
            MINIRE_INVARIANT(pointLightsShadowMaps.size() <= Ubo::maxPointLights(),
                             "provided {} shadow maps for point lights, while limit is {}",
                             pointLightsShadowMaps.size(), Ubo::maxPointLights());
            std::array<GLint, Ubo::maxPointLights()> pointLightsSamplers;
            for(size_t i = 0; i < pointLightsSamplers.size(); ++i)
            {
                pointLightsSamplers[i] = textureUnitHelper.activateNextUnit();
                if (i < pointLightsShadowMaps.size() &&
                    pointLightsShadowMaps[i])
                {
                    pointLightsShadowMaps[i]->bind();
                }
                else
                {
                    MINIRE_GL(glBindTexture, GL_TEXTURE_CUBE_MAP, 0);
                }
            }
            _program.setUniform(_pointLightsShadowMapsUniform, pointLightsSamplers);
        }

        // data for skinning
        if (!skinningVector.empty())
        {
            if (_bonesUniform != -1)
            {
                assert(skinningVector.size() <= rasterizer::Constants::kMaxBones);
                assert(_modelUniform == -1);
                _program.setUniform(_bonesUniform, skinningVector);
            }
        }
        else
        {
            assert(_modelUniform != -1);
        }

        // object picking buffer
        if (_meshIdUniform != -1) _program.setUniform(_meshIdUniform, meshId);
   }

    void Materials::Brush::setupCustomUniforms(TextureUnitHelper & textureUnitHelper) const
    {
        assert(_material);
        _material->updateUserUniforms(_userUniforms);

        assert(_userUniformMeta.size() == _userUniforms.size());
        for(size_t i = 0; i < _userUniforms.size(); ++i)
        {
            material::UserUniform & userUniform = _userUniforms[i];
            bool const updated = userUniform.updated();

            UserUniformMeta & userUniformMeta = _userUniformMeta[i];
            GLint const location = userUniformMeta._location;

            std::visit(utils::Overloaded
            {
                [location, updated, this](bool v)                { if (updated) _program.setUniform(location, v); },
                [location, updated, this](int32_t v)             { if (updated) _program.setUniform(location, v); },
                [location, updated, this](uint32_t v)            { if (updated) _program.setUniform(location, v); },
                [location, updated, this](float v)               { if (updated) _program.setUniform(location, v); },
                [location, updated, this](glm::vec2 const & v)   { if (updated) _program.setUniform(location, v); },
                [location, updated, this](glm::vec3 const & v)   { if (updated) _program.setUniform(location, v); },
                [location, updated, this](glm::vec4 const & v)   { if (updated) _program.setUniform(location, v); },
                [location, updated, this](glm::mat4 const & v)   { if (updated) _program.setUniform(location, v); },

                // NOTE: texture binding code is irrespective to "updated" because:
                //          - textures must be bound to texture units (since they are global states of OpenGL)
                //          - textureUnitHelper must be advanced to provide deterministic textures indexing
                [location, this, i, &textureUnitHelper, &userUniformMeta]
                (material::TextureUniform const & textureUniformSource)
                {
                    // maybe preload a texture
                    if (!userUniformMeta._texture)
                    {
                        assert(userUniformMeta._contentId.empty());
                        userUniformMeta._texture = _textures.get(textureUniformSource._textureId,
                                                                 textureUniformSource._sampler);
                        MINIRE_INVARIANT(userUniformMeta._texture, "failed to load texture: {}",
                                         textureUniformSource._textureId);
                        userUniformMeta._contentId = textureUniformSource._textureId;
                    }

                    // little sanity check
                    assert(userUniformMeta._texture);
                    assert(userUniformMeta._contentId == textureUniformSource._textureId);

                    // activate texture unit and bind a texture
                    textureUnitHelper.setTextureUniform(location, *userUniformMeta._texture);
                },

                [location, updated, this](std::array<glm::mat4, 6> const & v)
                {
                    if (updated) _program.setUniform(location, v);
                },

                // TODO: support all types, maybe just implement setUniform(material::Value const &)
                [](auto const & v)
                {
                    using T = std::decay_t<decltype(v)>;
                    MINIRE_THROW("unsupported type: {}", utils::demangle<T>());
                } 
            }, userUniform.value());
            userUniform.markDone();
        }
    }

    void Materials::Brush::prepareDrawing(glm::mat4 const & modelTransform,
                                          glm::vec3 const & ambientLight,
                                          glm::vec3 const & emissiveFactor,
                                          material::TextureRefs const & directionalLightsShadowMaps,
                                          material::TextureRefs const & pointLightsShadowMaps,
                                          material::SkinningVector const & skinningVector,
                                          uint32_t const meshId) const
    {
        _program.use();

        TextureUnitHelper textureUnitHelper(_program);

        setupBuiltinUniforms(textureUnitHelper, modelTransform, ambientLight, emissiveFactor,
                             directionalLightsShadowMaps, pointLightsShadowMaps, skinningVector, meshId);
 
        setupCustomUniforms(textureUnitHelper);
    }

    // Materials //

    Materials::Materials(Textures const & textures,
                         Ubo const & ubo)
        : _textures(textures)
        , _ubo(ubo)
    {}

    std::unique_ptr<nlohmann::json>
    Materials::makeBasicTemplateParams(models::MeshFeatures const & meshFeatures,
                                       std::string const & slug,
                                       std::string const & name) const
    {
        return std::make_unique<nlohmann::json>(nlohmann::json::object(
        {
            // fragment output locations
            {"minire_color_output_location",    0},    // vec3, +1
            {"minire_mesh_id_output_location",  1},    // uint, +1

            // constraints and limits
            {"minire_max_bones",                rasterizer::Constants::kMaxBones},
            {"minire_max_directional_lights",   Ubo::maxDirectionalLights()},
            {"minire_max_points_lights",        Ubo::maxPointLights()},

            // mesh features
            {"minire_mesh_traits",
                nlohmann::json::object(
                {
                    {"has_uv", meshFeatures.hasUv()},
                    {"has_normal", meshFeatures.hasNormal()},
                    {"has_tangent", meshFeatures.hasTangent()},
                    {"has_skin", meshFeatures.hasSkin()},
                })
            },

            // debug info
            {"minire_material_slug", slug},
            {"minire_material_name", name},
        }));
    }

    // IMPORTANT NOTE: for attribs and uniforms the increment of location MAY be more than 1 slot,
    //                 more specifically: matrixes, arrays, doubles (double, dvec).
    //                 Safe types are: float, int, uint, bool,
    //                                 (i/b/u)vec2, (i/b/u)vec3, (i/b/u)vec4 

    Materials::Brush::Sptr Materials::getBrush(models::MeshFeatures const & meshFeatures,
                                               Material::Sptr const & material) const
    {
        MINIRE_INVARIANT(material, "no material provided");

        // look up for a cached program
        std::string const & materialSlug = material->slug();
        Key const key(materialSlug, meshFeatures);
        if (auto it = _store.find(key); it != _store.cend())
        {
            if (Brush::Sptr cached = it->second.lock(); cached)
            {
                return cached;
            }
            else
            {
                // clean the cache from an expired record
                _store.erase(it);
            }
        }

        // build a program's code
        material::Program program = material->render();

        // build template args
        std::unique_ptr<nlohmann::json> templateParams = makeBasicTemplateParams(meshFeatures, materialSlug,
                                                                                 material->humanReadableName());
        if (program._extra)
        {
            assert(templateParams);
            templateParams->emplace("minire_extra", std::move(*program._extra));
            program._extra = {}; // moved out! must not be used anymore!
        }

        // prepare environment
        inja::Environment env;
        TemplateRenderingOutput outputs;
        // NOTE inja guarantees that inja::Arguments::at() will never return a nullptr.
        env.add_void_callback("minire_assert", 2, [](inja::Arguments const & args)
            {
                assert(args.size() == 2);
                if (!args.at(0)->get<bool>())
                {
                    MINIRE_THROW("minire_assert: {}", args.at(1)->get<std::string>());
                }
            });

        // Vertex attributes

        env.add_void_callback("minire_set_vertex_attrib_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._attribNames._vertex); });

        env.add_void_callback("minire_set_uv_attrib_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._attribNames._uv); });

        env.add_void_callback("minire_set_normal_attrib_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._attribNames._normal); });

        env.add_void_callback("minire_set_tangent_attrib_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._attribNames._tangent); });

        env.add_void_callback("minire_set_joints_attrib_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._attribNames._joints); });

        env.add_void_callback("minire_set_weights_attrib_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._attribNames._weights); });

        // UBO

        env.add_void_callback("minire_set_ubo_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._uboName); });

        // Uniforms

        env.add_void_callback("minire_set_model_uniform_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._builtinUniforms._model); });

        env.add_void_callback("minire_set_bones_uniform_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._builtinUniforms._bones); });

        env.add_void_callback("minire_set_directional_lights_shadow_maps_uniform_name", 1,
                              [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._builtinUniforms._directionalLightsShadowMaps); });

        env.add_void_callback("minire_set_point_lights_shadow_maps_uniform_name", 1,
                              [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._builtinUniforms._pointLightsShadowMaps); });

        env.add_void_callback("minire_set_emissive_factor_uniform_name", 1,
                              [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._builtinUniforms._emissiveFactor); });

        env.add_void_callback("minire_set_ambient_light_uniform_name", 1,
                              [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._builtinUniforms._ambientLight); });

        env.add_void_callback("minire_set_mesh_id_uniform_name", 1,
                              [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._builtinUniforms._meshId); });

        env.add_void_callback("minire_set_ubo_name", 1, [&outputs](inja::Arguments const & args)
                              { setSetOnce(args, outputs._uboName); });

        env.add_void_callback("minire_register_user_uniform", 1, [&outputs](inja::Arguments const & args)
        {
            assert(args.size() == 1);
            std::string const & name = args.at(0)->get<std::string>();
            auto [_, inserted] = outputs._userUniforms.emplace(name);
            MINIRE_INVARIANT(inserted, "failed to insert user's uniform (a duplicate?): \"{}\"", name);
        });

        // Guards

        env.add_void_callback("minire_set_guard", 1, [&outputs](inja::Arguments const & args)
        {
            assert(args.size() == 1);
            std::string const & name = args.at(0)->get<std::string>();
            outputs._activeGuards.emplace(name);
        });

        env.add_callback("minire_is_guard_set", 1, [&outputs](inja::Arguments const & args)
        {
            assert(args.size() == 1);
            std::string const & name = args.at(0)->get<std::string>();
            return outputs._activeGuards.contains(name);
        });

        // Includes

        env.include_template("minire/preamble.incl", env.parse(materials::preambleTemplate()));
        env.include_template("minire/ubo.incl", env.parse(materials::uboTemplate()));
        env.include_template("minire/attributes.incl", env.parse(materials::attributesTemplate()));
        env.include_template("minire/transform.incl", env.parse(materials::transformTemplate()));
        env.include_template("minire/uniforms.incl", env.parse(materials::uniformsTemplate()));
        for(auto const & [name, contents] : program._includes)
        {
            env.include_template(fmt::format("user/{}", name), env.parse(contents));
        }

        // render shaders's templates
        material::Shaders shadersSources;
        assert(program._shaders.size() == static_cast<size_t>(material::ShaderType::__kCount__));
        for(size_t i = 0; i < program._shaders.size(); ++i)
        {
            try
            {
                assert(templateParams);
                templateParams->emplace("minire_shader_type",
                                        material::toString(static_cast<material::ShaderType>(i)));
                shadersSources[i] = env.render(program._shaders[i], *templateParams);
                templateParams->erase("minire_shader_type");
            }
            catch(std::exception const & e)
            {
                MINIRE_THROW("Shader template rendering failed ({}):\n    {}\nTemplate parameters:\n{}",
                             material::toString(static_cast<material::ShaderType>(i)), e.what(),
                             templateParams->dump(4));
            }
        }

        // build the brush and memoize it in cache
        Brush::Sptr brush = std::make_shared<Brush>(_textures, _ubo, outputs,
                                                    std::move(shadersSources),
                                                    std::move(templateParams),
                                                    meshFeatures, material);
        MINIRE_DEBUG("a new brush issued (cache miss): material {}, slug: {}, h: {}",
                     material->humanReadableName(), key, std::hash<Key>{}(key));

        auto [_, inserted] = _store.emplace(key, brush);
        MINIRE_INVARIANT(inserted, "failed to insert a brush (a dup?), key: {}", key);
        return brush;
    }
}
