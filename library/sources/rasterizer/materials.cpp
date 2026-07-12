#include <rasterizer/materials.hpp>

#include <minire/content/id.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/overloaded.hpp>

#include <opengl/program.hpp>
#include <opengl/vao.hpp>
#include <opengl/vertex-buffer.hpp>
#include <rasterizer/binding-points.hpp>
#include <rasterizer/constants.hpp>
#include <rasterizer/materials/templates.hpp>
#include <rasterizer/mesh.hpp>
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
            std::optional<std::string> _ambientLight;
            std::optional<std::string> _emissiveFactor;
            std::optional<std::string> _meshId;
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

        // Will return a flat vector of subchunks of "maxBones" elements,
        // where each element a subchunk is 3 of glm::vec3 (0-th, 1-th,
        // and 2-nd rows of transform matrix; 3-rd row should reconsructed into glm::vec4(0, 0, 0, 1))
        std::vector<glm::vec4> flattenSkinningVectors
        (PrimitiveInstances::InstancedSkinningVectors const & isv, size_t const maxBones)
        {
            static const std::vector<glm::vec4> kFillers
            {
                glm::vec4(1, 0, 0, 0),
                glm::vec4(0, 1, 0, 0),
                glm::vec4(0, 0, 1, 0),
            };

            // NOTE: 'isv' is just a std::vector<std::vector<glm::mat4>>,
            //       inner vector's size must be no more than maxBones.
            std::vector<glm::vec4> result(maxBones * 3 * isv.size());
            size_t offset = 0;
            for(material::SkinningVectorSptr const & skinningVector : isv)
            {
                if (skinningVector)
                {
                    MINIRE_INVARIANT(skinningVector->size() <= maxBones,
                                     "too many bone: {} while the limit is {}",
                                     skinningVector->size(), maxBones);

                    // fill the matrices contents, glm::mat4 is column-major,
                    // so indeces are: matrix[column][row]
                    for(glm::mat4 const & matrix : *skinningVector)
                    {
                        glm::mat4 transposed = glm::transpose(matrix);
                        result[offset++] = transposed[0];
                        result[offset++] = transposed[1];
                        result[offset++] = transposed[2];
                        assert(transposed[3] == glm::vec4(0, 0, 0, 1));
                    }
                }
                else
                {
                    MINIRE_WARNING("mesh has skins, but no skinningVector provided");
                }

                // fill the gap
                size_t const residue = maxBones - (skinningVector ? skinningVector->size() : 0);
                for(size_t i = 0; i < residue; ++i)
                {
                    result[offset++] = kFillers[0];
                    result[offset++] = kFillers[1];
                    result[offset++] = kFillers[2];
                }
            }

            assert(offset == result.size());
            return result;
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

        void setTextureUniform(GLint location, models::TextureHandle & texture)
        {
            assert(location != -1);
            GLint texUnit = activateNextUnit();
            _program.setUniform(location, texUnit);
            Materials::bindTexture(texture);
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
        std::optional<std::string>      _uboName;
        BuiltinUniformNames             _builtinUniforms;
        std::unordered_set<std::string> _userUniforms;
        std::unordered_set<std::string> _activeGuards;
    };

    // Materials::Brush //

    Materials::Brush::Brush(Materials const & materials,
                            Textures const & textures,
                            Ubo const & ubo,
                            InstancedBuffersPool & instancedBuffersPool,
                            Materials::TemplateRenderingOutput const & tro,
                            material::Shaders && sources,
                            std::unique_ptr<nlohmann::json> && templateParams,
                            models::MeshFeatures const & meshFeatures)
        : _materials(materials)
        , _textures(textures)
        , _instancedBuffersPool(instancedBuffersPool)
        , _sources(std::move(sources))
        , _templateParams(std::move(templateParams))
        , _meshFeatures(meshFeatures)
        , _program(makeShaders(_sources, *_templateParams))
        , _directionalLightsShadowMapsUniform(_program.getUniformLocation(tro._builtinUniforms._directionalLightsShadowMaps))
        , _pointLightsShadowMapsUniform(_program.getUniformLocation(tro._builtinUniforms._pointLightsShadowMaps))
        , _ambientLightUniform(_program.getUniformLocation(tro._builtinUniforms._ambientLight))
        , _userUniformNames(tro._userUniforms.cbegin(), tro._userUniforms.cend())
        , _userUniformLocations([this] // NOTE: must be initialized AFTER _userUniformNames and _program
            {
                std::vector<GLint> result;
                result.reserve(_userUniformNames.size());
                for(std::string const & userUniformName : _userUniformNames)
                {
                    GLint const location = _program.getUniformLocation(userUniformName);
                    MINIRE_INVARIANT(location >= 0, "failed to resolve uniform location: \"{}\"",
                                     userUniformName);
                    result.emplace_back(location);
                }
                return result;
            }())
    {
        assert(_templateParams); // a bit late, but still

        // setup UBO
        if (tro._uboName)
        {
            _program.use();
            ubo.bindBufferRange(_program, *tro._uboName);
        }
    }

    void Materials::Brush::setupBuiltinUniforms(TextureUnitHelper & textureUnitHelper,
                                                glm::vec3 const & ambientLight,
                                                material::TextureRefs const & directionalLightsShadowMaps,
                                                material::TextureRefs const & pointLightsShadowMaps) const
    {
        if (_ambientLightUniform != -1) _program.setUniform(_ambientLightUniform, ambientLight);

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
    }

    void Materials::Brush::setupCustomUniforms(Material const & material,
                                               TextureUnitHelper & textureUnitHelper) const
    {
        // fetch value for user uniforms
        material::UniformValues const & uniformValues =
            material.updateUserUniforms(_userUniformNames, _textures);
        MINIRE_INVARIANT(uniformValues.size() == _userUniformNames.size(),
                         "user uniforms size mismatch: expected {}, but {} provided",
                         _userUniformNames.size(), uniformValues.size());

        // setup use uniform values
        assert(_userUniformNames.size() == _userUniformLocations.size());
        for(size_t i = 0; i < _userUniformLocations.size(); ++i)
        {
            GLint const location = _userUniformLocations[i];
            std::visit(utils::Overloaded
            {
                [location, this](bool v)                { _program.setUniform(location, v); },
                [location, this](int32_t v)             { _program.setUniform(location, v); },
                [location, this](uint32_t v)            { _program.setUniform(location, v); },
                [location, this](float v)               { _program.setUniform(location, v); },
                [location, this](glm::vec2 const & v)   { _program.setUniform(location, v); },
                [location, this](glm::vec3 const & v)   { _program.setUniform(location, v); },
                [location, this](glm::vec4 const & v)   { _program.setUniform(location, v); },
                [location, this](glm::mat4 const & v)   { _program.setUniform(location, v); },

                [location, this, &textureUnitHelper]
                (models::TextureHandle::Sptr const & textureHandle)
                {
                    MINIRE_INVARIANT(textureHandle, "empty textureHandle provided");
                    textureUnitHelper.setTextureUniform(location, *textureHandle);
                },

                [location, this](std::array<glm::mat4, 6> const & v)
                {
                    _program.setUniform(location, v);
                },

                // TODO: support all types, maybe just implement setUniform(material::Value const &)
                [](auto const & v)
                {
                    using T = std::decay_t<decltype(v)>;
                    MINIRE_THROW("unsupported type: {}", utils::demangle<T>());
                } 
            }, uniformValues[i]);
        }
    }

    void Materials::Brush::draw(UniquePrimitive const & uniquePrimitive,
                                PrimitiveInstances const & primitiveInstances,
                                glm::vec3 const & ambientLight,
                                material::TextureRefs const & directionalLightsShadowMaps,
                                material::TextureRefs const & pointLightsShadowMaps,
                                Material::Sptr const & materialOverride) const
    {
        _program.use();

        TextureUnitHelper textureUnitHelper(_program);

        // setup common uniforms
        setupBuiltinUniforms(textureUnitHelper, ambientLight, directionalLightsShadowMaps,
                             pointLightsShadowMaps);
        setupCustomUniforms(materialOverride ? *materialOverride
                                             : uniquePrimitive._mesh.material(uniquePrimitive._primitiveIndex),
                            textureUnitHelper);

        // fetch VertexBuffer
        opengl::VertexBuffer const & vertexBuffer =
            uniquePrimitive._mesh.vertexBuffer(uniquePrimitive._primitiveIndex);

        // perform drawing
        // fetch VAO and additional VBO
        opengl::VAO const & vao = vertexBuffer._vao;
        vao.bind();

        auto & fencedBuffers = _instancedBuffersPool.fetch();
        opengl::VBO & vbo = fencedBuffers->_vbo;
        opengl::SSBO & ssbo = fencedBuffers->_ssbo;

        // upload VBO's contents
        vbo.bind();
        assert(primitiveInstances.size() == primitiveInstances._attribs.size());
        if (GLsizeiptr const requiredSize = static_cast<GLsizeiptr>(
                primitiveInstances.size() * sizeof(PrimitiveInstances::Attribs));
            vbo.size() < requiredSize)
        {
            vbo.bufferData(requiredSize, primitiveInstances._attribs.data(), GL_DYNAMIC_DRAW);
        }
        else
        {
            vbo.bufferSubData(0, requiredSize, primitiveInstances._attribs.data());
        }

        // setup attributes in the VAO
        GLsizei const stride = static_cast<GLsizei>(sizeof(PrimitiveInstances::Attribs));

        if (_materials._modelAttrib != -1)
        {
           for (int i = 0; i < 4; ++i)
           {
                GLint const location = _materials._modelAttrib + i;
                vao.enableAttrib(location);
                size_t const offset = offsetof(PrimitiveInstances::Attribs, _transform) +
                                      (i * sizeof(glm::vec4)); // NOTE: mat4 = 4 * vec4
                vao.attribPointer(location, 4, GL_FLOAT, GL_FALSE, stride, offset);
                vao.attribDivisor(location, 1);
           }
        }

        if (_materials._emissiveFactorAttrib != -1)
        {
            vao.enableAttrib(_materials._emissiveFactorAttrib);
            vao.attribPointer(_materials._emissiveFactorAttrib, 3, GL_FLOAT, GL_FALSE, stride,
                              offsetof(PrimitiveInstances::Attribs, _emissiveFactor));
            vao.attribDivisor(_materials._emissiveFactorAttrib, 1);
        }

        if (_materials._meshIdAttrib != -1)
        {
            vao.enableAttrib(_materials._meshIdAttrib);
            vao.attribIPointer(_materials._meshIdAttrib, 1, GL_UNSIGNED_INT, stride,
                               offsetof(PrimitiveInstances::Attribs, _opbId));
            vao.attribDivisor(_materials._meshIdAttrib, 1);
        }

        // setup skinning vectors
        if (_meshFeatures.hasSkin())
        {
            assert(primitiveInstances.size() == primitiveInstances._skinningVectors.size());
            std::vector<glm::vec4> skinningVectors = flattenSkinningVectors(primitiveInstances._skinningVectors,
                                                                            rasterizer::Constants::kMaxBones);
            if (GLsizeiptr const requiredSize = static_cast<GLsizeiptr>(
                    skinningVectors.size() * sizeof(glm::vec4));
                ssbo.size() < requiredSize)
            {
                ssbo.bufferData(requiredSize, skinningVectors.data(), GL_DYNAMIC_DRAW);
            }
            else
            {
                ssbo.bufferSubData(0, requiredSize, skinningVectors.data());
            }
            ssbo.bindBufferBase(kSkinningVectrorBindingPoint);
        }

        // draw elements
        vertexBuffer.drawElementsInstanced(primitiveInstances.size());

        // create a fence
        GLsync fence = ::glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        MINIRE_MAYBE_THROW_GL(glFenceSync);
        fencedBuffers.setFence(fence);
    }

    // Materials //

    Materials::Materials(Textures const & textures,
                         Ubo const & ubo,
                         InstancedBuffersPool & instancedBuffersPool)
        : _textures(textures)
        , _ubo(ubo)
        , _instancedBuffersPool(instancedBuffersPool)
        , _locationsAllocator()
        , _attribLocations(_locationsAllocator.allocate(1), // vertex, vec3
                           _locationsAllocator.allocate(1), // uv, vec2
                           _locationsAllocator.allocate(1), // normal, vec3
                           _locationsAllocator.allocate(1), // tangent, vec3
                           _locationsAllocator.allocate(1), // joints, uvec4
                           _locationsAllocator.allocate(1)) // weights, vec4
        , _modelAttrib(_locationsAllocator.allocate(4))             // mat4
        , _emissiveFactorAttrib(_locationsAllocator.allocate(1))    // vec3
        , _meshIdAttrib((_locationsAllocator.allocate(1)))          // uvec
    {}

    std::unique_ptr<nlohmann::json>
    Materials::makeBasicTemplateParams(models::MeshFeatures const & meshFeatures,
                                       std::string const & slug,
                                       std::string const & name) const
    {
        return std::make_unique<nlohmann::json>(nlohmann::json::object(
        {
            // vertex attribute
            {"minire_vertex_attrib_location", _attribLocations.vertexAttribute()},
            {"minire_uv_attrib_location", _attribLocations.uvAttribute()},
            {"minire_normal_attrib_location", _attribLocations.normalAttribute()},
            {"minire_tangent_attrib_location", _attribLocations.tangentAttribute()},
            {"minire_joints_attrib_location", _attribLocations.jointsAttribute()},
            {"minire_weights_attrib_location", _attribLocations.weightsAttribute()},
            {"minire_instanced_model_attrib_location", _modelAttrib},
            {"minire_instanced_emissive_factor_attrib_location", _emissiveFactorAttrib},
            {"minire_instanced_mesh_id_attrib_location", _meshIdAttrib},

            // SSBOs
            {"minire_skinning_ssbo_binding_point", kSkinningVectrorBindingPoint},

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
                    {"has_uv",      meshFeatures.hasUv()},
                    {"has_normal",  meshFeatures.hasNormal()},
                    {"has_tangent", meshFeatures.hasTangent()},
                    {"has_skin",    meshFeatures.hasSkin()},
                })
            },

            // debug info
            {"minire_material_slug", slug},
            {"minire_material_name", name},
        }));
    }

    void Materials::bindTexture(models::TextureHandle const & textureHandle)
    {
        textureHandle.bind();
    }

    // IMPORTANT NOTE: for attribs and uniforms the increment of location MAY be more than 1 slot,
    //                 more specifically: matrixes, arrays, doubles (double, dvec).
    //                 Safe types are: float, int, uint, bool,
    //                                 (i/b/u)vec2, (i/b/u)vec3, (i/b/u)vec4 

    // NOTE: since decision to enable instanced rendering is currently depends only on
    //       meshFeatures, it is not required to add instancing flag into Materials::Key.

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

        // UBO

        env.add_void_callback("minire_set_ubo_name", 1, [&outputs](inja::Arguments const & args)
            { setSetOnce(args, outputs._uboName); });

        // Uniforms

        env.add_void_callback("minire_set_directional_lights_shadow_maps_uniform_name", 1,
            [&outputs](inja::Arguments const & args)
            { setSetOnce(args, outputs._builtinUniforms._directionalLightsShadowMaps); });

        env.add_void_callback("minire_set_point_lights_shadow_maps_uniform_name", 1,
            [&outputs](inja::Arguments const & args)
            { setSetOnce(args, outputs._builtinUniforms._pointLightsShadowMaps); });

        env.add_void_callback("minire_set_ambient_light_uniform_name", 1,
            [&outputs](inja::Arguments const & args)
            { setSetOnce(args, outputs._builtinUniforms._ambientLight); });

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
        Brush::Sptr brush = std::make_shared<Brush>(*this, _textures, _ubo, _instancedBuffersPool,
                                                    outputs, std::move(shadersSources),
                                                    std::move(templateParams),
                                                    meshFeatures);
        MINIRE_DEBUG("a new brush issued (cache miss): material {}, slug: {}, h: {}",
                     material->humanReadableName(), key, std::hash<Key>{}(key));

        auto [_, inserted] = _store.emplace(key, brush);
        MINIRE_INVARIANT(inserted, "failed to insert a brush (a dup?), key: {}", key);

        return brush;
    }
}
