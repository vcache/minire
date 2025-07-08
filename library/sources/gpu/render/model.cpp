#include <gpu/render/model.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <gpu/render/constants.hpp>
#include <gpu/render/ubo.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <utils/obj-to-index-buffers.hpp>
#include <utils/overloaded.hpp>

#include <inja/inja.hpp>
#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr

    // TODO: For release: (1) don't use OBJ instead just load dump from file
    //                    (2) don't realloc intermediate buffer for each mesh,
    //                        reuse it for each mesh

#include <cassert>
#include <variant>

namespace minire::gpu::render
{
    static const int kVertexAttr = 0;
    static const int kUvAttr = 1;
    static const int kNormalAttr = 2;
    static const int kTangentAttr = 3;

    static const char * kVertShader = R"(
        #version 330 core

        layout(location = {{ kVertexAttr }}) in vec3 bznkVertex;

        {% if kHasUvs %}
        layout(location = {{ kUvAttr }}) in vec2 bznkUv;
        out vec2 bznkFragUv;
        {% endif %}

        {% if kHasNormals %}
        layout(location = {{ kNormalAttr }}) in vec3 bznkNormal;
        out vec3 bznkFragNormal;
        {% endif %}

        {% if kHasTangents %}
        layout(location = {{ kTangentAttr }}) in vec3 bznkTangent;
        out mat3 bznkTbn;
        {% endif %}

        out vec4 bznkWorldPos;

        uniform mat4 bznkModel;

        {{ kUboDatablock }}

        void main()
        {
            bznkWorldPos = bznkModel * vec4(bznkVertex, 1.0);
            gl_Position = _viewProjection * bznkWorldPos;

            {% if kHasUvs %}
            bznkFragUv = bznkUv;
            {% endif %}

            {% if kHasNormals %}
            vec3 N = normalize(vec3(bznkModel * vec4(bznkNormal, 0.0)));
            bznkFragNormal = N;
            {% endif %}

            {% if kHasTangents %}
            vec3 T = normalize(vec3(bznkModel * vec4(bznkTangent, 0.0)));
            T = normalize(T - dot(T, N) * N);
            vec3 B = cross(N, T);
            bznkTbn = mat3(T, B, N);
            {% endif %}
        }
    )";

    static const char * kFragShader = R"(
        #version 330 core

        // output //

        out vec3 bznkOutColor;

        // input //

        {% if kHasUvs %}
        in vec2 bznkFragUv;
        {% endif %}

        {% if kHasNormals %}
        in vec3 bznkFragNormal;
        {% endif %}

        {% if kHasTangents %}
        in mat3 bznkTbn;
        {% endif %}

        in vec4 bznkWorldPos;

        // uniforms //

        {{ kUboDatablock }}

        {{ mkUniformDef(kAlbedoUniform, "bznkAlbedo") }}
        {{ mkUniformDef(kMetallicUniform, "bznkMetallic") }}
        {{ mkUniformDef(kRoughnessUniform, "bznkRoughness") }}
        {{ mkUniformDef(kAoUniform, "bznkAo") }}
        {{ mkUniformDef(kNormalsUniform, "bznkNormals") }}

        uniform float bznkColorFactor = 1.0;

        // routines //

        {% include "shaders/pbr-kit.incl" %}

        // entry pony //

        void main()
        {
            vec3 albedo = {{ mkValueSampler(kAlbedoUniform, "bznkAlbedo", 0) }};
            float metallic = {{ mkValueSampler(kMetallicUniform, "bznkMetallic", 1) }};
            float roughness = {{ mkValueSampler(kRoughnessUniform, "bznkRoughness", 1) }};
            float ao = {{ mkValueSampler(kAoUniform, "bznkAo", 1) }};
            
            {% if kHasNormalsUniform and kHasTangents %}
            vec3 normal = normalMapping(
                bznkTbn,
                {{ mkValueSampler(kNormalsUniform, "bznkNormals", 2) }});
            {% else %}
            vec3 normal = bznkFragNormal;
            {% endif %}

            bznkOutColor = pbrFragColor(albedo,
                                        metallic,
                                        roughness,
                                        ao,
                                        normal);

            bznkOutColor *= bznkColorFactor;
        }
    )";

    namespace
    {
        struct Params
        {
            bool   _hasUvs;
            bool   _hasNormals;
            bool   _hasTangents;
            size_t _albedoUniform;
            size_t _metallicUniform;
            size_t _roughnessUniform;
            size_t _aoUniform;
            size_t _normalsUniform;
        };

        auto makeShader(GLenum type,
                        char const * source,
                        Params const & params)
        {
            nlohmann::json vars {
                {"kVertexAttr",          kVertexAttr},
                {"kUvAttr",              kUvAttr},
                {"kNormalAttr",          kNormalAttr},
                {"kTangentAttr",         kTangentAttr},
                
                {"kHasUvs",              params._hasUvs},
                {"kHasNormals",          params._hasNormals},
                {"kHasTangents",         params._hasTangents},
                
                {"kHasAlbedoUniform",    0 != params._albedoUniform},
                {"kHasMetallicUniform",  0 != params._metallicUniform},
                {"kHasRoughnessUniform", 0 != params._roughnessUniform},
                {"kHasAoUniform",        0 != params._aoUniform},
                {"kHasNormalsUniform",   0 != params._normalsUniform},

                {"kAlbedoUniform",       params._albedoUniform},
                {"kMetallicUniform",     params._metallicUniform},
                {"kRoughnessUniform",    params._roughnessUniform},
                {"kAoUniform",           params._aoUniform},
                {"kNormalsUniform",      params._normalsUniform},

                {"kUboDatablock",        Ubo::interfaceBlock()},
            };

            inja::Environment env;
            env.include_template("shaders/pbr-kit.incl",
                                 env.parse(Constants::kPbrKit));

            env.add_callback("mkUniformDef", 2, [](inja::Arguments const & args)
                {
                    size_t const code = args.at(0)->get<size_t>();
                    std::string const & id = args.at(1)->get<std::string>();
                    switch(code)
                    {
                        case 0: return std::string();
                        case 1: return "uniform float " + id + ";";
                        case 2: return "uniform vec3 " + id + ";";
                        case 3: return "uniform sampler2D " + id + ";";
                    }
                    MINIRE_THROW("bad uniform kind code: {} for \"{}\"", code, id);
                });

            env.add_callback("mkValueSampler", 3, [](inja::Arguments const & args)
                {
                    size_t const code = args.at(0)->get<size_t>();
                    std::string const & id = args.at(1)->get<std::string>();
                    size_t const texMod = args.at(2)->get<size_t>();
                    switch(code)
                    {
                        case 0: return std::string(texMod == 2 ? "vec3(0)" : "0.0");
                        case 1: return id;
                        case 2: return id;
                        case 3:
                            if (texMod == 0) {
                                return "pow(texture(" + id + ", bznkFragUv).rgb, vec3(2.2))";
                            } else if (texMod == 1) {
                                return "texture(" + id + ", bznkFragUv).r";
                            } else if (texMod == 2) {
                                return "texture(" + id + ", bznkFragUv).rgb";
                            }
                    }
                    MINIRE_THROW("bad uniform kind code: {}", code);
                });

            std::string code = env.render(source, vars);

            // TODO: make sure shaders won't print in release
            //LOGD("shader generated:" << code);

            return std::make_shared<opengl::Shader>(type, code);
        }

        class ModelProgram : public Model::Program
        {
        public:
            ModelProgram(Params const & params,
                         Ubo const & ubo)
                : _program({
                    makeShader(GL_VERTEX_SHADER, kVertShader, params),
                    makeShader(GL_FRAGMENT_SHADER, kFragShader, params),
                })
                , _uniforms
                {
                    params._albedoUniform    ? _program.getUniformLocation("bznkAlbedo")    : -1,
                    params._metallicUniform  ? _program.getUniformLocation("bznkMetallic")  : -1,
                    params._roughnessUniform ? _program.getUniformLocation("bznkRoughness") : -1,
                    params._aoUniform        ? _program.getUniformLocation("bznkAo")        : -1,
                    params._normalsUniform   ? _program.getUniformLocation("bznkNormals")   : -1
                }
                , _modelUniform(_program.getUniformLocation("bznkModel"))
                , _colorFactorUniform(_program.getUniformLocation("bznkColorFactor"))
            {
                _program.use();
                ubo.bindBufferRange(_program);
            }

            void use() const override { _program.use(); }

            bool isUsing() const { return _program.isUsing(); }

        public:
            void setUniform1f(float v, size_t i) const
            {
                assert(i < _uniforms.size());
                assert(_uniforms[i] >= 0);
                MINIRE_GL(glUniform1f, _uniforms[i], v);
            }

            void setUniform3f(glm::vec3 const & v, size_t i) const
            {
                assert(i < _uniforms.size());
                assert(_uniforms[i] >= 0);
                MINIRE_GL(glUniform3f, _uniforms[i], v.x, v.y, v.z);
            }

            void setUniform1i(GLint v, size_t i) const
            {
                assert(i < _uniforms.size());
                assert(_uniforms[i] >= 0);
                MINIRE_GL(glUniform1i, _uniforms[i], v);
            }

            void setModelTransform(glm::mat4 const & m) const
            {
                MINIRE_GL(glUniformMatrix4fv, _modelUniform, 1, GL_FALSE, glm::value_ptr(m));
            }

            void setColorFactorUniform(float v) const
            {
                MINIRE_GL(glUniform1f, _colorFactorUniform, v);
            }

        private:
            opengl::Program      _program;
            std::array<GLint, 5> _uniforms;
            GLint                _modelUniform;
            GLint                _colorFactorUniform;
        };

    } // namespace

    void Model::Mapper::setupUniforms(Program const & prog) const
    {
        auto const & program = static_cast<ModelProgram const &>(prog);
        switch(_kind)
        {
            case Kind::kNone:
                break;

            case Kind::kFloat:
                program.setUniform1f(_float, _index);
                break;

            case Kind::kVector3:
                program.setUniform3f(_vector3, _index);
                break;

            case Kind::kTexture:
                MINIRE_GL(glActiveTexture, GL_TEXTURE0 + _index);
                program.setUniform1i(_index, _index);
                _texture->bind();
                break;
        }
    }

    namespace
    {
        // TODO: don't reload meshes that is already loaded
        auto makeBuffers(content::Id const & id,
                         content::Manager & contentManager)
        {
            auto lease = contentManager.borrow(id);
            assert(lease);
            if (formats::Obj const * obj = lease->tryAs<formats::Obj>())
            {
                MINIRE_INFO("Loading OBJ-mesh: {}", id);;
                return utils::createIndexBuffers(*obj, kVertexAttr, kUvAttr, kNormalAttr);
            }
            else
            {
                MINIRE_THROW("unknown mesh format: \"{}\"", id);
            }
        }
    }

    Model::Mapper Model::mapToMapper(Textures const & textures,
                                     models::SceneModel::Map const & map,
                                     size_t index)
    {
        return std::visit(utils::Overloaded
        {
            [index](std::monostate)                   { return Mapper(index); },
            [index](float v)                          { return Mapper(v, index); },
            [index](glm::vec3 const & v)              { return Mapper(v, index); },
            [&textures, index](content::Id const & v) { return Mapper(textures.get(v), index); },
        }, map);
    }

    Model::Model(models::SceneModel const & sceneModel,
                 content::Manager & contentManager,
                 Textures const & textures,
                 Ubo const & ubo)
        : _ubo(ubo)
        , _buffers(makeBuffers(sceneModel._mesh, contentManager))
        , _albedo(mapToMapper(textures, sceneModel._albedo, 0))
        , _metallic(mapToMapper(textures, sceneModel._metallic, 1))
        , _roughness(mapToMapper(textures, sceneModel._roughness, 2))
        , _ao(mapToMapper(textures, sceneModel._ao, 3))
        , _normals(mapToMapper(textures, sceneModel._normals, 4))
        , _flags(buildFlags()) // NOTE: _must_ be latest
    {
        MINIRE_INVARIANT(_albedo.kind() != Mapper::Kind::kNone, "albedo required: {}", sceneModel._mesh);
        MINIRE_INVARIANT(_albedo.kind() != Mapper::Kind::kFloat, "albedo cannot be float: {}", sceneModel._mesh);
        MINIRE_INVARIANT(_normals.kind() != Mapper::Kind::kFloat, "normals cannot be float: {}", sceneModel._mesh);
        MINIRE_INVARIANT(_normals.kind() != Mapper::Kind::kVector3, "normals cannot be vec3: {}", sceneModel._mesh);
    }

    size_t Model::buildFlags() const
    {
        size_t result(0);

        if (_buffers.hasUvs())      result |= (1 << 0);
        if (_buffers.hasNormals())  result |= (1 << 1);
        if (_buffers.hasTangents()) result |= (1 << 2);
        // reserved for bitangent (1 << 3)

        result |= (static_cast<size_t>(_albedo.kind())    << (4 + 0));
        result |= (static_cast<size_t>(_metallic.kind())  << (4 + 2));
        result |= (static_cast<size_t>(_roughness.kind()) << (4 + 4));
        result |= (static_cast<size_t>(_ao.kind())        << (4 + 6));
        result |= (static_cast<size_t>(_normals.kind())   << (4 + 8));

        return result;
    }

    Model::Program::Sptr Model::makeProgram() const
    {
        return std::make_shared<ModelProgram>(
            Params{
                _buffers.hasUvs(),
                _buffers.hasNormals(),
                _buffers.hasTangents(),
                static_cast<size_t>(_albedo.kind()),
                static_cast<size_t>(_metallic.kind()),
                static_cast<size_t>(_roughness.kind()),
                static_cast<size_t>(_ao.kind()),
                static_cast<size_t>(_normals.kind())
            },
            _ubo);
    }

    void Model::draw(Program const & prog,
                     glm::mat4 const & modelTransform,
                     float const colorFactor) const
    {
        // TODO: pass bone deform

        auto const & program = static_cast<ModelProgram const &>(prog);
        assert(program.isUsing());

        program.setModelTransform(modelTransform);
        program.setColorFactorUniform(colorFactor);

        _albedo.setupUniforms(program);
        _metallic.setupUniforms(program);
        _roughness.setupUniforms(program);
        _ao.setupUniforms(program);
        _normals.setupUniforms(program);

        _buffers.drawElements();
    }
}
