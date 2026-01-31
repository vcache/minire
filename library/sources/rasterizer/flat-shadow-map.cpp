#include <rasterizer/flat-shadow-map.hpp>

#include <opengl.hpp>
#include <opengl/shader.hpp>
#include <rasterizer/constants.hpp>
#include <rasterizer/mesh.hpp>
#include <rasterizer/shadow-map-program-key.hpp>
#include <utils/frustum.hpp>

#include <minire/errors.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <inja/inja.hpp>

#include <cassert>
#include <limits>
#include <tuple>

namespace minire::rasterizer
{
    class FlatShadowMap::Programs final
        : public opengl::ProgramCache<ShadowMapProgramKey>
    {
    public:
        constexpr static size_t kLightMatrix = 0;
        constexpr static size_t kModelMatrix = 1;
        constexpr static size_t kBonesMatrices = 2;

        Programs()
            : ProgramCache({kLightMatrix, kModelMatrix, kBonesMatrices})
        {}

    private:
        static constexpr auto kVertShader =
        R"(
            #version 330 core

            in vec3 bznkVertex;

            {% include "shaders/model-skinning-kit.incl" %}

            uniform mat4 bznkLightMatrix;

            void main()
            {
                mat4 effectiveModel = getEffectiveModelMatrix();
                gl_Position = bznkLightMatrix * effectiveModel * vec4(bznkVertex, 1.0);
            }
        )";

        static constexpr auto kFragShader =
        R"(
            #version 330 core
            void main() {}
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

            Shaders::UniformCodes uniformCodes{kLightMatrix};
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
                default: MINIRE_THROW("bad uniform code: {}", code);
            }
        }
    };

    // FlatShadowMap //

    FlatShadowMap::FlatShadowMap(size_t size)
        : _size(size)
        , _programs(std::make_unique<Programs>())
        , _texture(GL_TEXTURE_2D)
        , _fbo()
    {
        MINIRE_INVARIANT(size <= std::numeric_limits<GLsizei>::max(),
                         "too huge size: {}", size);

        MINIRE_GL(glTexImage2D, GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                  _size, _size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        _texture.parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        _texture.parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        _texture.parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        _texture.parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        static float const kBorderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        MINIRE_GL(glTexParameterfv, GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, kBorderColor);
    }

    FlatShadowMap::~FlatShadowMap() = default;

    glm::mat4 FlatShadowMap::buildVP(glm::vec3 const & lightPosition,
                                     glm::vec3 const & lightDirection,
                                     utils::FrustumVertices const & frustumVertices) const
    {
        // build view matrix
        glm::vec3 upVector = glm::abs(glm::dot(lightDirection, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f
            ? glm::vec3(0.0f, 0.0f, 1.0f)
            : glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 view = glm::lookAt(lightPosition,
                                     lightPosition + lightDirection * 10.0f,
                                     upVector);

        // build projection matrix
        glm::vec3 minExtent(std::numeric_limits<float>::max());
        glm::vec3 maxExtent(std::numeric_limits<float>::min());

        for (glm::vec3 const & vertex : frustumVertices)
        {
            glm::vec4 const lightSpaceCorner = view * glm::vec4(vertex, 1.0f);

            minExtent.x = std::min(minExtent.x, lightSpaceCorner.x);
            maxExtent.x = std::max(maxExtent.x, lightSpaceCorner.x);
            minExtent.y = std::min(minExtent.y, lightSpaceCorner.y);
            maxExtent.y = std::max(maxExtent.y, lightSpaceCorner.y);
            minExtent.z = std::min(minExtent.z, lightSpaceCorner.z);
            maxExtent.z = std::max(maxExtent.z, lightSpaceCorner.z);
        }

        maxExtent.z += 50.0f;

        glm::mat4 const projection = glm::ortho(minExtent.x, maxExtent.x,
                                                minExtent.y, maxExtent.y,
                                                minExtent.z, maxExtent.z);

        return projection * view;
    }

    glm::mat4 FlatShadowMap::perform(CulledPrimitives const & primitives,
                                     glm::vec3 const & lightPosition,
                                     glm::vec3 const & lightDirection,
                                     utils::FrustumVertices const & frustumVertices)
    {
        // setup GL mode flags
        MINIRE_GL(glEnable, GL_DEPTH_TEST);
        MINIRE_GL(glDepthFunc, GL_LESS);
        MINIRE_GL(glDepthMask, GL_TRUE);
        //MINIRE_GL(glDisable, GL_DEPTH_CLAMP);

#       if 0
        // TODO: (it solves Peter Panning, but kills floor-like flat planes)
        MINIRE_GL(glEnable, GL_CULL_FACE);
        MINIRE_GL(glCullFace, GL_FRONT);
#       endif

        // bind texture to the depth framebuffer
        _fbo.attach2D(_texture, GL_DEPTH_ATTACHMENT);
        MINIRE_GL(glDrawBuffer, GL_NONE);
        MINIRE_GL(glReadBuffer, GL_NONE);

        // setup canvas
        MINIRE_GL(glViewport, 0, 0, _size, _size);
        MINIRE_GL(glClear, GL_DEPTH_BUFFER_BIT);

        // evaluate light VP
        glm::mat4 const lightVP = buildVP(lightPosition,
                                          glm::normalize(lightDirection),
                                          frustumVertices);

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
        for(auto const & [programKey, primitives] : drawQueue)
        {
            auto const & program = _programs->getUsingProgram(programKey);
            program.setUniformByCode(Programs::kLightMatrix, lightVP);
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
        return lightVP;
    }
}
