#include <rasterizer/cube-shadow-map.hpp>

#include <codegen/factory.hpp>
#include <codegen/plugins/skinning.hpp>
#include <codegen/plugins/vertex-position.hpp>
#include <rasterizer/mesh.hpp>
#include <scene-impl.hpp>
#include <utils/overloaded.hpp>

#include <minire/errors.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <inja/inja.hpp>

#include <array>
#include <limits>
#include <tuple>

// TODO: a lot of code duplicated between CubeShadowMap and FlatShadowMap

namespace minire::rasterizer
{
    class CubeShadowMap::Factory
        : public codegen::CachedFactory<codegen::plugins::VertexPosition,
                                        codegen::plugins::Skinning>
    {
        static constexpr auto kVertShader =
        R"(
            #version 330 core

            {% include "shaders/vertex-position.incl" %}

            {% include "shaders/model-skinning-kit.incl" %}

            void main()
            {
                mat4 effectiveModel = getEffectiveModelMatrix();
                gl_Position = effectiveModel * vec4({{ bznkVertex }}, 1.0);
            }
        )";

        static constexpr auto kGeomShader =
        R"(
            #version 330 core

            layout (triangles) in;
            layout (triangle_strip, max_vertices=18) out;

            uniform mat4 bznkShadowMatrices[6];

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
            #version 330 core

            in vec4 FragPos;

            uniform vec3 bznkLightPos;
            uniform float bznkFarPlane;

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
            #version 330 core

            layout (location = 0) out float outputRed32;

            in vec4 FragPos;

            uniform vec3 bznkLightPos;
            uniform float bznkFarPlane;
            uniform float kFactor;

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
            #version 330 core

            layout (location = 0) out float outputRed32;

            in vec4 FragPos;

            uniform vec3 bznkLightPos;
            uniform float bznkFarPlane;
            uniform float kFactor;

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

    public:
        explicit Factory(models::shadow_params::Method const & method)
            : CachedFactory({
                {GL_VERTEX_SHADER, kVertShader},
                {GL_FRAGMENT_SHADER, fetchFragShader(method)},
                {GL_GEOMETRY_SHADER, kGeomShader},
            })
            , _shadowMatrices(getOrMakeUniformCode("bznkShadowMatrices"))
            , _lightPos(getOrMakeUniformCode("bznkLightPos"))
            , _farPlane(getOrMakeUniformCode("bznkFarPlane"))
            , _kFactor(getOrMakeUniformCode("kFactor"))
        {
            assert(_shadowMatrices != -1);
            assert(_lightPos != -1);
            assert(_farPlane != -1);
            // NOTE: _kFactor might be -1
        }

        GLint const _shadowMatrices = -1;
        GLint const _lightPos = -1;
        GLint const _farPlane = -1;
        GLint const _kFactor = -1;
    };

    // CubeShadowMap //

    CubeShadowMap::CubeShadowMap(models::ShadowParams const & shadowParams)
        : _shadowParams(shadowParams)
        , _factory(std::make_unique<Factory>(_shadowParams._method))
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

    CubeShadowMap::~CubeShadowMap() = default;

    namespace
    {
        using CubeVPs = std::array<glm::mat4, 6>;

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
        std::unordered_map<codegen::Traits, std::vector<CulledPrimitive const *>> drawQueue;
        for(CulledPrimitive const & primitive : primitives)
        {
            auto const & [_, attribLocations] =
                primitive._mesh.primitiveTraits(primitive._primitiveIndex);
            codegen::Traits traits{attribLocations};
            drawQueue[traits].emplace_back(&primitive);
        }

        // perform drawing commands
        assert(_factory);
        for(auto const & [traits, primitives] : drawQueue)
        {
            // fetch a program for given traits
            auto const & program = _factory->getUsingProgram(traits);

            // setup uniforms
            program.setUniformByCode(_factory->_shadowMatrices, lightVPs);
            program.setUniformByCode(_factory->_lightPos, lightPosition);
            program.setUniformByCode(_factory->_farPlane, far);
            std::visit(utils::Overloaded
            {
                [](models::shadow_params::method::Standard const &) {},
                [&program, this](models::shadow_params::method::ESM const & esm)
                {
                    assert(_factory->_kFactor != -1);
                    program.setUniformByCode(_factory->_kFactor, esm._factor);
                },
                [&program, this](models::shadow_params::method::LogESM const & logEsm)
                {
                    assert(_factory->_kFactor != -1);
                    program.setUniformByCode(_factory->_kFactor, logEsm._factor);
                },
            }, _shadowParams._method);

            // perform drawing operations
            for(CulledPrimitive const * primitive : primitives)
            {
                assert(primitive);
                program.setSkinningUniforms(primitive->_transform, primitive->_skinningVector);
                primitive->_mesh.drawBare(primitive->_primitiveIndex);
            }
        }

        // tidy up
        _fbo.unbind();

        return far;
    }
}
