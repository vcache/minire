#include <rasterizer/flat-shadow-map.hpp>

#include <rasterizer/filters/gaussian-blur.hpp>
#include <rasterizer/mesh.hpp>
#include <utils/frustum.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/material.hpp>
#include <minire/utils/overloaded.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/mat4x4.hpp>
#include <inja/inja.hpp>

#include <cassert>
#include <limits>
#include <optional>

namespace minire::rasterizer
{
    class FlatShadowMap::Material
        : public ::minire::Material
    {
        static constexpr auto kVertShader =
        R"(
            {% include "minire/preamble.incl" %}
            {% include "minire/attributes.incl" %}
            {% include "minire/transform.incl" %}

            uniform mat4 bznkLightMatrix; {{ minire_register_user_uniform("bznkLightMatrix") }}

            void main()
            {
                mat4 effectiveModel = minireModelMatrix();
                gl_Position = bznkLightMatrix * effectiveModel * vec4(minireVertex, 1.0);
            }
        )";

        static constexpr auto kFragShaderStd =
        R"(
            {% include "minire/preamble.incl" %}
            void main() {}
        )";

        static constexpr auto kFragShaderESM =
        R"(
            {% include "minire/preamble.incl" %}
            layout (location = 0) out float outputRed32;

            uniform float kFactor; {{ minire_register_user_uniform("kFactor") }}

            void main()
            {
                // NOTE: gl_FragCoord.z is linear since
                // we use orthographic projection
                outputRed32 = exp(kFactor * gl_FragCoord.z);
            }
        )";

        static constexpr auto kFragShaderLogESM =
        R"(
            {% include "minire/preamble.incl" %}
            layout (location = 0) out float outputRed32;

            uniform float kFactor; {{ minire_register_user_uniform("kFactor") }}

            void main()
            {
                // NOTE: gl_FragCoord.z is linear since
                // we use orthographic projection
                outputRed32 = kFactor * gl_FragCoord.z;
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

        struct UniformIndeces
        {
            static constexpr size_t kNoIndex = std::numeric_limits<size_t>::max();

            size_t _kFactor         = kNoIndex;
            size_t _bznkLightMatrix = kNoIndex;

            explicit UniformIndeces(material::UniformNames const & names)
            {
                for(size_t i = 0; i < names.size(); ++i)
                {
                    std::string const & name = names[i];

                    if ("kFactor" == name)
                    {
                        _kFactor = i;
                    }
                    else if ("bznkLightMatrix" == name)
                    {
                        _bznkLightMatrix = i;
                    }
                    else
                    {
                        MINIRE_THROW("unexpected user uniform: \"{}\"", name);
                    }
                }
            }
        };

    public:
        material::Program render() const override
        {
            material::Shaders shaders;
            shaders[static_cast<int>(material::ShaderType::kVertex)] = kVertShader;
            shaders[static_cast<int>(material::ShaderType::kFragment)] = fetchFragShader(_method);

            return material::Program
            {
                ._shaders = std::move(shaders),
                ._extra = {},
                ._includes = {},
            };
        }

        material::UniformValues const &
        updateUserUniforms(material::UniformNames const & uniformsNames,
                           models::TextureResolver const &) const override
        {
            if (!_uniformIndeces) _uniformIndeces.emplace(uniformsNames);
            _uniformValues.resize(uniformsNames.size());

            assert(_uniformIndeces);

            // kFactor
            if (_uniformIndeces->_kFactor != UniformIndeces::kNoIndex)
            {
                assert(_uniformIndeces->_kFactor < _uniformValues.size());
                _uniformValues[_uniformIndeces->_kFactor] = _factor;
            }

            // bznkLightMatrix
            assert(_uniformIndeces->_bznkLightMatrix < _uniformValues.size());
            _uniformValues[_uniformIndeces->_bznkLightMatrix] = _lightMatrix;

            return _uniformValues;
        }

        std::string slugImpl() const override
        {
            return fmt::format("m:{}:{}", fetchSlug(_method), _instanceKey);
        }

    public:
        explicit Material(models::shadow_params::Method const & method,
                          size_t instanceKey)
            : _method(method)
            , _instanceKey(instanceKey)
        {}

        void setFactor(float v) { _factor = v; }
        void setLightMatrix(glm::mat4 const & v) { _lightMatrix = v; }

    private:
        models::shadow_params::Method const   _method;
        size_t const                          _instanceKey;

        mutable std::optional<UniformIndeces> _uniformIndeces;
        mutable material::UniformValues       _uniformValues;

        float                                 _factor;
        glm::mat4                             _lightMatrix;
    };

    // FlatShadowMap //

    FlatShadowMap::FlatShadowMap(Materials const & materials,
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

        MINIRE_INVARIANT(_shadowParams._mapSize <= std::numeric_limits<GLsizei>::max(),
                         "too huge size: {}", _shadowParams._mapSize);

        // setup mandatory depth map

        {
            _depthTexture = std::make_unique<opengl::Texture>(GL_TEXTURE_2D); // will be bound

            MINIRE_GL(glTexImage2D, GL_TEXTURE_2D, 0,
                      _shadowParams._zBuffer32 ? GL_DEPTH_COMPONENT32F
                                               : GL_DEPTH_COMPONENT,
                      _shadowParams._mapSize, _shadowParams._mapSize, 0,
                      GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            _depthTexture->parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            _depthTexture->parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            _depthTexture->parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            _depthTexture->parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            static float const kBorderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
            MINIRE_GL(glTexParameterfv, GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, kBorderColor);

            _fbo.attach2D(*_depthTexture, GL_DEPTH_ATTACHMENT);
        }

        // setup optional secondary map

        if (std::holds_alternative<method::ESM>(_shadowParams._method) ||
            std::holds_alternative<method::LogESM>(_shadowParams._method))
        {
            _shadowTexture = std::make_unique<opengl::Texture>(GL_TEXTURE_2D); // will be bound

            MINIRE_GL(glTexImage2D, GL_TEXTURE_2D, 0, GL_R32F,
                      _shadowParams._mapSize, _shadowParams._mapSize, 0,
                      GL_RED, GL_FLOAT, nullptr);

            _shadowTexture->parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            _shadowTexture->parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            _shadowTexture->parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            _shadowTexture->parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            static float const kBorderColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
            MINIRE_GL(glTexParameterfv, GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, kBorderColor);

            _fbo.attach2D(*_shadowTexture, GL_COLOR_ATTACHMENT0);
        }
        else
        {
            MINIRE_GL(glDrawBuffer, GL_COLOR_ATTACHMENT0);
            MINIRE_GL(glReadBuffer, GL_COLOR_ATTACHMENT0);
        }

        // ensure FBO state

        MINIRE_INVARIANT(::glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                         "shadow FBO isn't complete");

        // setup the filter (if any)

        if (std::holds_alternative<filter::GaussianBlur>(_shadowParams._filter))
        {
            if(_shadowTexture)
            {
                _gaussianBlur = std::make_unique<filters::GaussianBlur>(_shadowParams._mapSize,
                                                                        _shadowParams._mapSize,
                                                                        GL_R32F);
            }
            else
            {
                MINIRE_WARNING("GaussianBlur cannot be applied to a chosen shadow map method");
            }
        }
    }

    // TODO: it should clean up cached brushes (see extraBrush())
    FlatShadowMap::~FlatShadowMap() = default;

    glm::mat4 FlatShadowMap::buildVP(glm::vec3 const & /*lightPosition*/,
                                     glm::vec3 const & lightDirection,
                                     utils::ViewFrustum const & viewFrustum) const
    {
        using namespace models::shadow_params;

        std::optional<glm::vec3> frustumCenter;

        // eval center of a frustum
        glm::vec3 center = std::visit(utils::Overloaded
        {
            [](center::Absolute const & absolute) { return absolute._value; },
            [&viewFrustum, &frustumCenter](center::Frustum const &)
            {
                frustumCenter = viewFrustum.center();
                return *frustumCenter;
            },
            [&viewFrustum](center::CameraYPlaneHitPoint const &)
            {
                return viewFrustum.directionYHitPoint();
            },
        }, _shadowParams._center);

        // eval raduis of a circumscribed circle
        // (to prevent shadow jitter during camera rotation/movement).
        // Note, it won't helpt if FOV or near plane changed
        float radius = 0.0f;

        if (!std::holds_alternative<margin::Absolute>(_shadowParams._radiusMargin))
        {
            if (!frustumCenter) frustumCenter = viewFrustum.center();
            for (glm::vec3 const & v : viewFrustum)
            {
                radius = std::max(radius, glm::distance(*frustumCenter, v));
            }
        }

        std::visit(utils::Overloaded
        {
            [](std::monostate const &) { },
            [&radius](margin::Absolute const & absolute) { radius = absolute._value; },
            [&radius](margin::Constant const & constant) { radius += constant._value; },
            [&radius](margin::Factor const & factor) { radius *= factor._value; },
        }, _shadowParams._radiusMargin);

        radius = std::ceil(radius);

        // build the view matrix
        glm::vec3 upVector = glm::abs(glm::dot(lightDirection, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f
            ? glm::vec3(0.0f, 0.0f, 1.0f)
            : glm::vec3(0.0f, 1.0f, 0.0f);
        auto lightView = glm::lookAt(center - lightDirection * radius, center, upVector);

        // evaluate far and near planes to be used for projection matrix
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();

        if (!std::holds_alternative<std::monostate>(_shadowParams._nearMargin) ||
            !std::holds_alternative<std::monostate>(_shadowParams._farMargin))
        {
            for (glm::vec3 const & v : viewFrustum)
            {
                glm::vec4 const p = lightView * glm::vec4(v, 1.0f);
                minZ = std::min(minZ, p.z);
                maxZ = std::max(maxZ, p.z);
            }
        }

        // adjust near plane
        std::visit(utils::Overloaded
        {
            [&minZ](std::monostate const &) { minZ = 0.0f; },
            [&minZ](margin::Absolute const & absolute) { minZ = absolute._value; },
            [&minZ](margin::Constant const & constant) { minZ -= constant._value; },
            [&minZ](margin::Factor const & factor)
            {
                if (minZ < 0) { minZ *= factor._value; } else { minZ /= factor._value; }
            },
        }, _shadowParams._nearMargin);

        // adjust far plane
        std::visit(utils::Overloaded
        {
            [&maxZ, radius](std::monostate const &) { maxZ = 2.0f * radius; },
            [&maxZ](margin::Absolute const & absolute) { maxZ = absolute._value; },
            [&maxZ](margin::Constant const & constant) { maxZ += constant._value; },
            [&maxZ](margin::Factor const & factor)
            {
                if (maxZ < 0) { maxZ /= factor._value; } else { maxZ *= factor._value; }
            },
        }, _shadowParams._farMargin);

        // calculate projection matrix
        glm::mat4 const lightProjection = glm::ortho(-radius, radius, -radius, radius, minZ, maxZ);

        // recalculate lightView matrix w.r.t. per-texel alignment
        glm::vec4 lookAtOrigin = lightView * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        float const texelsPerUnit = static_cast<float>(_shadowParams._mapSize) / (radius * 2.0f);
        lookAtOrigin *= texelsPerUnit;
        glm::vec3 const roundedOrigin = glm::floor(glm::vec3(lookAtOrigin));
        glm::vec3 const roundOffset = (roundedOrigin - glm::vec3(lookAtOrigin)) / texelsPerUnit;
        lightView = glm::translate(glm::mat4(1.0f), glm::vec3(roundOffset.x, roundOffset.y, 0.0f)) * lightView;

        // calculate resulting matrix
        return lightProjection * lightView;
    }

    glm::mat4 FlatShadowMap::perform(CulledPrimitives const & primitives,
                                     glm::vec3 const & lightPosition,
                                     glm::vec3 const & lightDirection,
                                     utils::ViewFrustum const & viewFrustum)
    {
        // setup GL mode flags
        MINIRE_GL(glEnable, GL_DEPTH_TEST);
        MINIRE_GL(glDepthFunc, GL_LESS);
        MINIRE_GL(glDepthMask, GL_TRUE);
        MINIRE_GL(glDisable, GL_BLEND);

#       if 0
        // TODO: (it solves Peter Panning, but kills floor-like flat planes)
        MINIRE_GL(glEnable, GL_CULL_FACE);
        MINIRE_GL(glCullFace, GL_FRONT);
#       endif

        // bind the FBO
        _fbo.bind();

        // setup canvas
        MINIRE_GL(glViewport, 0, 0, _shadowParams._mapSize, _shadowParams._mapSize);
        MINIRE_GL(glClearColor, 0.0f, 0.0f, 0.0f, 1.0f);
        MINIRE_GL(glClear, GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        // evaluate light VP
        glm::mat4 const lightVP = buildVP(lightPosition,
                                          glm::normalize(lightDirection),
                                          viewFrustum);

        // setup material-specific unifroms
        assert(_material);
        _material->setLightMatrix(lightVP);
        std::visit(utils::Overloaded
        {
            [this](models::shadow_params::method::Standard const &) { },
            [this](models::shadow_params::method::ESM const & v)    { _material->setFactor(v._factor); },
            [this](models::shadow_params::method::LogESM const & v) { _material->setFactor(v._factor); },
        }, _shadowParams._method);

        // perform the drawing
        for(auto const & [uniquePrimitive, primitiveInstances] : primitives)
        {
            Materials::Brush::Sptr & brush = uniquePrimitive._mesh.extraBrush(
                uniquePrimitive._primitiveIndex, _meshConsumerKey);
            if (!brush)
            {
                models::MeshFeatures const & meshFeatures =
                    uniquePrimitive._mesh.meshFeatures(uniquePrimitive._primitiveIndex);
                brush = _materials.getBrush(meshFeatures, _material);
            }

            assert(brush);
            brush->draw(uniquePrimitive, primitiveInstances,
                        glm::vec3() /* ambientLight */,
                        {} /* directionalLightsShadowMaps */,
                        {} /* pointLightsShadowMaps */,
                        _material);
        }

        // tidy up
        _fbo.unbind();

        // apply a filter (if any)
        if (_gaussianBlur)
        {
            assert(_shadowTexture);

            using namespace models::shadow_params::filter;
            assert(std::holds_alternative<GaussianBlur>(_shadowParams._filter));
            GaussianBlur const & gaussianBlur = std::get<GaussianBlur>(_shadowParams._filter);
            for (size_t i = 0; i < gaussianBlur._iterations; i++)
            {
                _gaussianBlur->perform(*_shadowTexture);
            }
        }

        return lightVP;
    }
}
