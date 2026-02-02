#include <rasterizer/flat-shadow-map.hpp>

#include <codegen/factory.hpp>
#include <codegen/plugins/skinning.hpp>
#include <codegen/plugins/vertex-position.hpp>
#include <rasterizer/mesh.hpp>
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
    class FlatShadowMap::Factory
        : public codegen::CachedFactory<codegen::plugins::VertexPosition,
                                        codegen::plugins::Skinning>
    {
        static constexpr auto kVertShader =
        R"(
            #version 330 core

            {% include "shaders/vertex-position.incl" %}

            {% include "shaders/model-skinning-kit.incl" %}

            uniform mat4 bznkLightMatrix;

            void main()
            {
                mat4 effectiveModel = getEffectiveModelMatrix();
                gl_Position = bznkLightMatrix * effectiveModel * vec4({{ bznkVertex }}, 1.0);
            }
        )";

        static constexpr auto kFragShader =
        R"(
            #version 330 core
            void main() {}
        )";

    public:
        Factory()
            : CachedFactory(
            {
                {GL_VERTEX_SHADER, kVertShader},
                {GL_FRAGMENT_SHADER, kFragShader}
            })
            , _lightMatrixCode(getOrMakeUniformCode("bznkLightMatrix"))
        {}

        size_t const _lightMatrixCode = -1;
    };

    // FlatShadowMap //

    FlatShadowMap::FlatShadowMap(size_t size)
        : _size(size)
        , _factory(std::make_unique<Factory>())
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
            auto const & program = _factory->getUsingProgram(traits);
            program.setUniformByCode(_factory->_lightMatrixCode, lightVP);
            for(CulledPrimitive const * primitive : primitives)
            {
                assert(primitive);
                program.setSkinningUniforms(primitive->_transform, primitive->_skinningVector);
                primitive->_mesh.drawBare(primitive->_primitiveIndex);
            }
        }

        // tidy up
        _fbo.unbind();
        return lightVP;
    }
}
