#include <rasterizer/cube-shadow-map.hpp>

#include <rasterizer/mesh.hpp>
#include <scene-impl.hpp>

#include <minire/errors.hpp>
#include <minire/material.hpp>
#include <minire/utils/overloaded.hpp>

#include <fmt/format.h>
#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <inja/inja.hpp>

#include <array>
#include <limits>
#include <tuple>

// TODO: a lot of code duplicated between CubeShadowMap and FlatShadowMap

namespace minire::rasterizer
{
    namespace
    {
        using CubeVPs = std::array<glm::mat4, 6>;
    }

    class CubeShadowMap::Material
        : public ::minire::Material
    {
        static constexpr auto kVertShader =
        R"(
            {% include "minire/preamble.incl" %}

            in vec3 minireVertex; {{ minire_set_vertex_attrib_name("minireVertex") }}

            {% include "minire/transform.incl" %}

            void main()
            {
                mat4 effectiveModel = minireModelMatrix();
                gl_Position = effectiveModel * vec4(minireVertex, 1.0);
            }
        )";

        static constexpr auto kGeomShader =
        R"(
            {% include "minire/preamble.incl" %}

            layout (triangles) in;
            layout (triangle_strip, max_vertices=18) out;

            uniform mat4 bznkShadowMatrices[6]; {{ minire_register_user_uniform("bznkShadowMatrices") }}

            out vec4 FragPos; // FragPos from GS (output per emitvertex)

            void main()
            {
                for(int face = 0; face < 6; ++face)
                {
                    gl_Layer = face; // built-in variable that specifies to which face we render.
                    for(int i = 0; i < 3; ++i) // for each triangle vertex
                    {
                        FragPos = gl_in[i].gl_Position;
                        gl_Position = bznkShadowMatrices[face] * FragPos;
                        EmitVertex();
                    }
                    EndPrimitive();
                }
            }
        )";

        static constexpr auto kFragShaderStd =
        R"(
            {% include "minire/preamble.incl" %}

            in vec4 FragPos;

            uniform vec3 bznkLightPos; {{ minire_register_user_uniform("bznkLightPos") }}
            uniform float bznkFarPlane; {{ minire_register_user_uniform("bznkFarPlane") }}

            void main()
            {
                // get distance between fragment and light source
                float lightDistance = length(FragPos.xyz - bznkLightPos);

                // map to [0;1] range by dividing by bznkFarPlane
                lightDistance = lightDistance / bznkFarPlane;

                // write this as modified depth
                gl_FragDepth = lightDistance;
            }
        )";

        static constexpr auto kFragShaderESM =
        R"(
            {% include "minire/preamble.incl" %}

            layout (location = 0) out float outputRed32;

            in vec4 FragPos;

            uniform vec3 bznkLightPos; {{ minire_register_user_uniform("bznkLightPos") }}
            uniform float bznkFarPlane; {{ minire_register_user_uniform("bznkFarPlane") }}
            uniform float kFactor; {{ minire_register_user_uniform("kFactor") }}

            void main()
            {
                // get distance between fragment and light source
                float lightDistance = length(FragPos.xyz - bznkLightPos);

                // map to [0;1] range by dividing by bznkFarPlane
                lightDistance = lightDistance / bznkFarPlane;

                // lightDistance is linearized, so it is legal
                outputRed32 = exp(kFactor * lightDistance);
            }
        )";

        static constexpr auto kFragShaderLogESM =
        R"(
            {% include "minire/preamble.incl" %}

            layout (location = 0) out float outputRed32;

            in vec4 FragPos;

            uniform vec3 bznkLightPos; {{ minire_register_user_uniform("bznkLightPos") }}
            uniform float bznkFarPlane; {{ minire_register_user_uniform("bznkFarPlane") }}
            uniform float kFactor; {{ minire_register_user_uniform("kFactor") }}

            void main()
            {
                // get distance between fragment and light source
                float lightDistance = length(FragPos.xyz - bznkLightPos);

                // map to [0;1] range by dividing by bznkFarPlane
                lightDistance = lightDistance / bznkFarPlane;

                // lightDistance is linearized, so it is legal
                outputRed32 = kFactor * lightDistance;
            }
        )";

        static auto fetchFragShader(models::shadow_params::Method const & method)
        {
            return std::visit(utils::Overloaded
            {
                [](models::shadow_params::method::Standard const &) { return kFragShaderStd; },
                [](models::shadow_params::method::ESM const &)      { return kFragShaderESM; },
                [](models::shadow_params::method::LogESM const &)   { return kFragShaderLogESM; },
            }, method);
        }

        static auto fetchSlug(models::shadow_params::Method const & method)
        {
            return std::visit(utils::Overloaded
            {
                [](models::shadow_params::method::Standard const &) { return "std"; },
                [](models::shadow_params::method::ESM const &)      { return "ESM"; },
                [](models::shadow_params::method::LogESM const &)   { return "LogESM"; },
            }, method);
        }

        struct Uniforms
        {
            static constexpr char kFarPlaneName[] = "bznkFarPlane";
            static constexpr char kFactorName[] = "kFactor";
            static constexpr char kLightPosName[] = "bznkLightPos";
            static constexpr char kShadowMatricesName[] = "bznkShadowMatrices";

            material::UserUniformTracker<float, kFarPlaneName>         _farPlane;
            material::UserUniformTracker<float, kFactorName>           _factor;
            material::UserUniformTracker<glm::vec3, kLightPosName>     _lightPos;
            material::UserUniformTracker<CubeVPs, kShadowMatricesName> _shadowMatrices;

            explicit Uniforms(material::UserUniforms & userUniforms)
                : _farPlane(userUniforms)
                , _factor(userUniforms)
                , _lightPos(userUniforms)
                , _shadowMatrices(userUniforms)
            {}
        };

    public:
        material::Program render() const override
        {
            material::Shaders shaders;
            shaders[static_cast<int>(material::ShaderType::kVertex)] = kVertShader;
            shaders[static_cast<int>(material::ShaderType::kFragment)] = fetchFragShader(_method);
            shaders[static_cast<int>(material::ShaderType::kGeometry)] = kGeomShader;

            return material::Program
            {
                ._shaders = std::move(shaders),
                ._extra = {},
                ._includes = {},
            };
        }

        void updateUserUniforms(material::UserUniforms & userUniforms) const override
        {
            Uniforms * uniforms = userUniforms.getOrMakeUserData<Uniforms>(userUniforms);
            assert(uniforms);

            uniforms->_farPlane.set(_farPlane);
            if (_factor) uniforms->_factor.set(*_factor);
            uniforms->_lightPos.set(_lightPos);
            uniforms->_shadowMatrices.set(_shadowMatrices);
        }

        std::string slugImpl() const override
        {
            return fmt::format("m:{}", fetchSlug(_method), _instanceKey);
        }

    public:
        explicit Material(models::shadow_params::Method const & method,
                          size_t instanceKey)
            : _method(method)
            , _instanceKey(instanceKey)
        {}

        // TODO: instead copying, maybe just update by reference values that alredy allocated?
        void setFarPlane(float value) { _farPlane = value; }
        void setFactor(float value) { _factor = value; }
        void unsetFactor() { _factor.reset(); }
        void setLightPos(glm::vec3 const & value) { _lightPos = value; }
        void setShadowMatrices(CubeVPs const & value) { _shadowMatrices = value; }

    private:
        models::shadow_params::Method const _method;
        size_t const                        _instanceKey;
        float                               _farPlane;
        std::optional<float>                _factor;
        glm::vec3                           _lightPos;
        CubeVPs                             _shadowMatrices;
    };

    // CubeShadowMap //

    CubeShadowMap::CubeShadowMap(Materials const & materials,
                                 models::ShadowParams const & shadowParams)
        : _materials(materials)
        , _shadowParams(shadowParams)
        , _meshConsumerKey(Mesh::issueConsumerKey())
        , _material(std::make_shared<Material>(_shadowParams._method, _meshConsumerKey))
        , _depthTexture()
        , _shadowTexture()
        , _fbo()
    {
        using namespace models::shadow_params;

        // _fbo will be attached

        MINIRE_INVARIANT(_shadowParams._mapSize  <= std::numeric_limits<GLsizei>::max(),
                         "too huge size: {}", _shadowParams._mapSize);

        // setup mandatory depth map

        {
            _depthTexture = std::make_unique<opengl::Texture>(GL_TEXTURE_CUBE_MAP); // will be bound

            for (size_t i = 0; i < 6; ++i)
            {
                MINIRE_GL(glTexImage2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                          _shadowParams._zBuffer32 ? GL_DEPTH_COMPONENT32F
                                                   : GL_DEPTH_COMPONENT,
                          _shadowParams._mapSize, _shadowParams._mapSize, 0,
                          GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            }

            // TODO: GL_CLAMP_TO_BORDER ?
            _depthTexture->parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            _depthTexture->parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            _depthTexture->parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            _depthTexture->parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            _depthTexture->parameteri(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            _fbo.attach(*_depthTexture, GL_DEPTH_ATTACHMENT);
        }

        // setup optional secondary map

        if (std::holds_alternative<method::ESM>(_shadowParams._method) ||
            std::holds_alternative<method::LogESM>(_shadowParams._method))
        {
            _shadowTexture = std::make_unique<opengl::Texture>(GL_TEXTURE_CUBE_MAP); // will be bound

            for (size_t i = 0; i < 6; ++i)
            {
                MINIRE_GL(glTexImage2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R32F,
                          _shadowParams._mapSize, _shadowParams._mapSize, 0,
                          GL_RED, GL_FLOAT, nullptr);
            }

            // TODO: GL_CLAMP_TO_BORDER ?
            _shadowTexture->parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            _shadowTexture->parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            _shadowTexture->parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            _shadowTexture->parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            _shadowTexture->parameteri(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            _fbo.attach(*_shadowTexture, GL_COLOR_ATTACHMENT0);
        }
        else
        {
            MINIRE_GL(glDrawBuffer, GL_COLOR_ATTACHMENT0);
            MINIRE_GL(glReadBuffer, GL_COLOR_ATTACHMENT0);
        }

        // ensure FBO state

        MINIRE_INVARIANT(::glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                         "shadow FBO isn't complete");

        // additional checks

        MINIRE_INVARIANT(std::holds_alternative<center::Frustum>(_shadowParams._center),
                         "point lights don't support center's customization (center::Frustum expected)");

        MINIRE_INVARIANT(std::holds_alternative<std::monostate>(_shadowParams._radiusMargin),
                         "point lights don't support radius margins");

        // TODO: why not?
        MINIRE_INVARIANT(std::holds_alternative<std::monostate>(_shadowParams._nearMargin),
                         "point lights don't support near margins");

        // TODO: why not?
        MINIRE_INVARIANT(std::holds_alternative<std::monostate>(_shadowParams._farMargin),
                         "point lights don't support far margins");

        MINIRE_INVARIANT(!std::holds_alternative<filter::GaussianBlur>(_shadowParams._filter),
                         "point lights don't support GaussianBlur");
    }

    // TODO: it should clean up cached brushes (see extraBrush())
    CubeShadowMap::~CubeShadowMap() = default;

    namespace
    {
        CubeVPs buildVPs(glm::vec3 const & lightPosition,
                         float const near,
                         float const far)
        {
            // build projection matrix
            float const aspect = 1.0f; // width/height, but they are the same
            glm::mat4 const projection = glm::perspective(glm::radians(90.0f),
                                                          aspect, near, far);

            // build VP matrices
            return CubeVPs
            {
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(1.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(-1.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(0.0f, 1.0f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, 1.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(0.0f, -1.0f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, -1.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(0.0f, 0.0f, 1.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f)),
                projection * glm::lookAt(lightPosition,
                                         lightPosition + glm::vec3(0.0f, 0.0f, -1.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f)),
            };
        }


        std::pair<float, float>
        calcNearFar(utils::ViewFrustum const & viewFrustum)
        {
            static float const kNear = 1.0f;

            glm::vec3 min = viewFrustum[0];
            glm::vec3 max = viewFrustum[0];
            for (glm::vec3 const & vertex : viewFrustum)
            {
                min = glm::min(min, vertex);
                max = glm::max(max, vertex);
            }

            float const far = glm::compMax(max - min);
            return std::make_pair(kNear, far);
        }
    }

    float CubeShadowMap::perform(CulledPrimitives const & primitives,
                                 glm::vec3 const & lightPosition,
                                 utils::ViewFrustum const & viewFrustum)
    {
        // setup GL mode flags
        MINIRE_GL(glEnable, GL_DEPTH_TEST);
        MINIRE_GL(glDepthFunc, GL_LESS);
        MINIRE_GL(glDepthMask, GL_TRUE);
        MINIRE_GL(glDisable, GL_BLEND);

        // bind the FBO
        _fbo.bind();

        // setup canvas
        MINIRE_GL(glViewport, 0, 0, _shadowParams._mapSize, _shadowParams._mapSize);
        MINIRE_GL(glClearColor, 0.0f, 0.0f, 0.0f, 1.0f);
        MINIRE_GL(glClear, GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        // calculate planes and light VP
        auto const [near, far] = calcNearFar(viewFrustum);
        CubeVPs const lightVPs = buildVPs(lightPosition, near, far);

        // collect programs
        using DrawQueue = std::unordered_map<models::MeshFeatures,
                                             std::vector<CulledPrimitive const *>>;
        DrawQueue drawQueue;
        for(CulledPrimitive const & primitive : primitives)
        {
            auto const & [meshFeatures, _] =
                primitive._mesh.primitiveTraits(primitive._primitiveIndex);
            drawQueue[meshFeatures].emplace_back(&primitive);
        }

        // perform drawing commands
        assert(_material);
        for(auto const & [meshFeatures, primitives] : drawQueue)
        {
            // setup uniforms
            _material->setShadowMatrices(lightVPs);
            _material->setLightPos(lightPosition);
            _material->setFarPlane(far);
            std::visit(utils::Overloaded
            {
                [this](models::shadow_params::method::Standard const &) { _material->unsetFactor(); },
                [this](models::shadow_params::method::ESM const & v)    { _material->setFactor(v._factor); },
                [this](models::shadow_params::method::LogESM const & v) { _material->setFactor(v._factor); },
            }, _shadowParams._method);

            // perform drawing operations
            for(CulledPrimitive const * primitive : primitives)
            {
                assert(primitive);

                // fetch or create a brush for given primitives
                Materials::Brush::Sptr & brush = primitive->_mesh.extraBrush(_meshConsumerKey);
                if (!brush)
                {
                    brush = _materials.getBrush(meshFeatures, _material);
                }
                assert(brush);

                // TODO: some parameters can be optional!
                brush->prepareDrawing(primitive->_transform,
                                      glm::vec3() /* ambientLight */,
                                      glm::vec3() /* emissiveFactor */,
                                      {} /* directionalLightsShadowMaps */,
                                      {} /* pointLightsShadowMaps */,
                                      primitive->_skinningVector,
                                      0 /* meshId */);
                primitive->_mesh.drawBare(primitive->_primitiveIndex);
            }
        }

        // tidy up
        _fbo.unbind();

        return far;
    }
}
