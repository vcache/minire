#include <rasterizer/cube-shadow-map.hpp>

#include <opengl.hpp>
#include <opengl/shader.hpp>
#include <rasterizer/constants.hpp>
#include <rasterizer/mesh.hpp>
#include <rasterizer/shadow-map-program-key.hpp>
#include <scene.hpp>

#include <minire/errors.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <inja/inja.hpp>

#include <array>
#include <limits>
#include <tuple>

namespace minire::rasterizer
{
    class CubeShadowMap::Programs
        : public opengl::ProgramCache<ShadowMapProgramKey>
    {
    public:
        constexpr static size_t kLightMatrix = 0;
        constexpr static size_t kModelMatrix = 1;
        constexpr static size_t kBonesMatrices = 2;
        constexpr static size_t kShadowMatrices = 3;
        constexpr static size_t kLightPos = 4;
        constexpr static size_t kFarPlane = 5;

        Programs()
            : ProgramCache({kLightMatrix, kModelMatrix, kBonesMatrices,
                            kShadowMatrices, kLightPos, kFarPlane})
        {}

    private:
        static constexpr auto kVertShader =
        R"(
            #version 330 core

            in vec3 bznkVertex;

            {% include "shaders/model-skinning-kit.incl" %}

            void main()
            {
                mat4 effectiveModel = getEffectiveModelMatrix();
                gl_Position = effectiveModel * vec4(bznkVertex, 1.0);
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

        static constexpr auto kFragShader =
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

        Shaders renderShaders(ShadowMapProgramKey const & programKey) const override
        {
            inja::Environment env;
            env.include_template("shaders/model-skinning-kit.incl",
                                 env.parse(Constants::kModelSkinningKit));
            nlohmann::json vars
            {
                {"kHasSkins", programKey.hasSkin()},
                {"kMaxBones", rasterizer::Constants::kMaxBones},
            };
            std::string vertShader = env.render(kVertShader, vars);

            Shaders::UniformCodes uniformCodes{kLightMatrix, kShadowMatrices, kLightPos, kFarPlane};
            uniformCodes.emplace(programKey.hasSkin() ? kBonesMatrices : kModelMatrix);

            assert(programKey._vertexLocation >= 0);
            Shaders::AttribLocations attribLocations{{"bznkVertex", programKey._vertexLocation}};
            if (programKey.hasSkin())
            {
                assert(programKey._jointsLocation >= 0);
                assert(programKey._weightsLocation >= 0);
                attribLocations.emplace("bznkJoints", programKey._jointsLocation);
                attribLocations.emplace("bznkWeights", programKey._weightsLocation);
            }

            return Shaders
            {
                ._sources = Shaders::Sources
                {
                    {GL_VERTEX_SHADER, vertShader},
                    {GL_FRAGMENT_SHADER, kFragShader},
                    {GL_GEOMETRY_SHADER, kGeomShader},
                },
                ._uniformCodes = uniformCodes,
                ._attribLocations = attribLocations,
            };
        }

        std::string getUniformName(size_t const code) const override
        {
            switch(code)
            {
                case kLightMatrix: return "bznkLightMatrix";
                case kModelMatrix: return "bznkModel";
                case kBonesMatrices: return "bznkBones";
                case kShadowMatrices: return "bznkShadowMatrices";
                case kLightPos: return "bznkLightPos";
                case kFarPlane: return "bznkFarPlane";
                default: MINIRE_THROW("bad uniform code: {}", code);
            }
        }
    };

    // CubeShadowMap //

    CubeShadowMap::CubeShadowMap(size_t size)
        : _size(size)
        , _programs(std::make_unique<Programs>())
        , _texture(GL_TEXTURE_CUBE_MAP)
        , _fbo()
    {
        MINIRE_INVARIANT(size <= std::numeric_limits<GLsizei>::max(),
                         "too huge size: {}", size);

        for (size_t i = 0; i < 6; ++i)
        {
            MINIRE_GL(glTexImage2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                      _size, _size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }

        // TODO: GL_CLAMP_TO_BORDER ?
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        MINIRE_GL(glTexParameteri, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
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
        calcNearFar(utils::FrustumVertices const & frustumVertices)
        {
            static float const kNear = 1.0f;

            glm::vec3 min = frustumVertices[0];
            glm::vec3 max = frustumVertices[0];
            for (glm::vec3 const & vertex : frustumVertices)
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
                                 utils::FrustumVertices const & frustumVertices)
    {
        // setup GL mode flags
        MINIRE_GL(glEnable, GL_DEPTH_TEST);
        MINIRE_GL(glDepthFunc, GL_LESS);
        MINIRE_GL(glDepthMask, GL_TRUE);

        // bind cube map to the depth framebuffer
        _fbo.attach(_texture, GL_DEPTH_ATTACHMENT);
        MINIRE_GL(glDrawBuffer, GL_NONE);
        MINIRE_GL(glReadBuffer, GL_NONE);

        // setup canvas
        MINIRE_GL(glViewport, 0, 0, _size, _size);
        MINIRE_GL(glClear, GL_DEPTH_BUFFER_BIT);

        // calculate planes and light VP
        auto const [near, far] = calcNearFar(frustumVertices);
        CubeVPs const lightVPs = buildVPs(lightPosition, near, far);

        // collect programs
        std::unordered_map<ShadowMapProgramKey, std::vector<CulledPrimitive const *>> drawQueue;
        for(CulledPrimitive const & primitive : primitives)
        {
            auto const & [meshFeatures, attribLocations] =
                primitive._mesh.primitiveTraits(primitive._primitiveIndex);
            ShadowMapProgramKey programKey(meshFeatures, attribLocations);
            drawQueue[programKey].emplace_back(&primitive);
        }

        // perform drawing commands
        MINIRE_GL(glActiveTexture, GL_TEXTURE0);
        _texture.bind();

        for(auto const & [programKey, primitives] : drawQueue)
        {
            auto const & program = _programs->getUsingProgram(programKey);
            program.setUniformByCode(Programs::kShadowMatrices, lightVPs);
            program.setUniformByCode(Programs::kLightPos, lightPosition);
            program.setUniformByCode(Programs::kFarPlane, far);

            for(CulledPrimitive const * primitive : primitives)
            {
                assert(primitive);
                if (programKey.hasSkin())
                {
                    assert(primitive->_skinningVector.size() <= rasterizer::Constants::kMaxBones);
                    program.setUniformByCode(Programs::kBonesMatrices, primitive->_skinningVector);
                }
                else
                {
                    program.setUniformByCode(Programs::kModelMatrix, primitive->_transform);
                }
                primitive->_mesh.drawBare(primitive->_primitiveIndex);
            }
        }

        // tidy up
        _fbo.unbind();
        return far;
    }
}