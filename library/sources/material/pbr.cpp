#include <minire/material/pbr.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <fmt/format.h>
#include <inja/inja.hpp>

namespace minire::material
{
    namespace
    {
        std::string toString(models::PbrMaterial::TextureComponent textureComponent)
        {
            switch(textureComponent)
            {
                case models::PbrMaterial::TextureComponent::kR: return "r";
                case models::PbrMaterial::TextureComponent::kG: return "g";
                case models::PbrMaterial::TextureComponent::kB: return "b";
                case models::PbrMaterial::TextureComponent::kA: return "a";
            }
            MINIRE_THROW("unexpected texture component: {}",
                         static_cast<int>(textureComponent));
        }

        // NOTE: these shaders are based on https://learnopengl.com/
        // TODO: move to a file and use #embed after migration to C++23
        static std::string const kPbrKit =
        R"(

            // PBR KIT BEGIN //

            const float PI = 3.14159265359;

            vec3 normalMapping(mat3 tbn, vec3 normal, float scale)
            {
                normal = normalize((normal * 2.0 - 1.0) * vec3(scale, scale, 1.0));
                return normalize(tbn * normal);
            }

            float DistributionGGX(vec3 N, vec3 H, float roughness)
            {
                float a = roughness*roughness;
                float a2 = a*a;
                float NdotH = max(dot(N, H), 0.0);
                float NdotH2 = NdotH*NdotH;

                float nom   = a2;
                float denom = (NdotH2 * (a2 - 1.0) + 1.0);
                denom = PI * denom * denom;

                return nom / max(denom, 0.001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
            }

            float GeometrySchlickGGX(float NdotV, float roughness)
            {
                float r = (roughness + 1.0);
                float k = (r*r) / 8.0;

                float nom   = NdotV;
                float denom = NdotV * (1.0 - k) + k;

                return nom / denom;
            }

            float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
            {
                float NdotV = max(dot(N, V), 0.0);
                float NdotL = max(dot(N, L), 0.0);
                float ggx2 = GeometrySchlickGGX(NdotV, roughness);
                float ggx1 = GeometrySchlickGGX(NdotL, roughness);

                return ggx1 * ggx2;
            }

            vec3 fresnelSchlick(float cosTheta, vec3 F0)
            {
                return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
            }

            vec3 calcLo(const vec3 N,
                        const vec3 V,
                        const vec3 L,
                        const vec3 H,
                        const vec3 F0,
                        const vec3 radiance,
                        const vec3 albedo,
                        const float roughness,
                        const float metallic)
            {
                // Cook-Torrance BRDF
                float NDF = DistributionGGX(N, H, roughness);
                float G   = GeometrySmith(N, V, L, roughness);
                vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

                vec3 nominator    = NDF * G * F;
                float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
                // prevent divide by zero for NdotV=0.0 or NdotL=0.0
                vec3 specular = nominator / max(denominator, 0.001);

                // kS is equal to Fresnel
                vec3 kS = F;
                // for energy conservation, the diffuse and specular light can't
                // be above 1.0 (unless the surface emits light); to preserve this
                // relationship the diffuse component (kD) should equal 1.0 - kS.
                vec3 kD = vec3(1.0) - kS;
                // multiply kD by the inverse metalness such that only non-metals
                // have diffuse lighting, or a linear blend if partly metal (pure metals
                // have no diffuse light).
                kD *= 1.0 - metallic;

                // scale light by NdotL
                float NdotL = max(dot(N, L), 0.0);

                // add to outgoing radiance Lo
                // note that we already multiplied the BRDF by the Fresnel (kS)
                // so we won't multiply by kS again
                return (kD * albedo / PI + specular) * radiance * NdotL;
            }

            float shadowFactorDirect_Std(const vec4 lightSpaceFragPos,
                                         const sampler2D shadowMap,
                                         const float bias)
            {
                vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;
                if (projCoords.z > 1.0)
                {
                    return 0.0;
                }
                projCoords = projCoords * 0.5 + 0.5;
                float currentDepth = projCoords.z - bias;
                float closestDepth = texture(shadowMap, projCoords.xy).r;
                return currentDepth > closestDepth  ? 1.0 : 0.0;
            }

            float shadowFactorDirect_Std_PCF(const vec4 lightSpaceFragPos,
                                             const sampler2D shadowMap,
                                             const float bias)
            {
                vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;
                if (projCoords.z > 1.0)
                {
                    return 0.0;
                }

                projCoords = projCoords * 0.5 + 0.5;
                float currentDepth = projCoords.z - bias;

                float shadow = 0.0;
                vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
                for(int x = -1; x <= 1; ++x)
                {
                    for(int y = -1; y <= 1; ++y)
                    {
                        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                        shadow += currentDepth > pcfDepth ? 1.0 : 0.0;
                    }
                }
                shadow /= 9.0;

                return shadow;
            }

            float shadowFactorDirect_ESM(const vec4 lightSpaceFragPos,
                                         const sampler2D shadowMap,
                                         const float bias,
                                         const float kFactor)
            {
                vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;
                if (projCoords.z > 1.0)
                {
                    return 0.0;
                }
                projCoords = projCoords * 0.5 + 0.5;
                float currentDepth = projCoords.z - bias;
                float occluder = texture(shadowMap, projCoords.xy).r;
                float shadow = occluder * exp(-kFactor * currentDepth);
                return clamp(shadow, 0.0, 1.0);
            }

            float shadowFactorDirect_LogESM(const vec4 lightSpaceFragPos,
                                            const sampler2D shadowMap,
                                            const float bias,
                                            const float kFactor)
            {
                vec3 projCoords = lightSpaceFragPos.xyz / lightSpaceFragPos.w;
                if (projCoords.z > 1.0)
                {
                    return 0.0;
                }
                projCoords = projCoords * 0.5 + 0.5;
                float occluder = texture(shadowMap, projCoords.xy).r;
                float currentDepth = projCoords.z - bias;
                float shadow = exp(occluder - (kFactor * currentDepth));
                return clamp(shadow, 0.0, 1.0);
            }

            float shadowFactorPoint_Std(const vec3 fragmentPos,
                                        const vec3 lightPos,
                                        const samplerCube shadowMap,
                                        const float shadowMapFarPlane,
                                        const float bias)
            {
                vec3 fragToLight = fragmentPos - lightPos;
                float closestDepth = texture(shadowMap, fragToLight).r;
                closestDepth *= shadowMapFarPlane;
                float currentDepth = length(fragToLight);
                return currentDepth - bias > closestDepth ? 1.0 : 0.0;
            }

            float shadowFactorPoint_Std_PCF(const vec3 fragmentPos,
                                            const vec3 lightPos,
                                            const samplerCube shadowMap,
                                            const float shadowMapFarPlane,
                                            const float bias)
            {
                const vec3 kSampleOffsetDirections[20] = vec3[]
                (
                   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
                   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
                   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
                   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
                   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
                );

                vec3 fragToLight = fragmentPos - lightPos;
                float currentDepth = length(fragToLight);

                float viewDistance = length(_viewPosition.xyz - fragmentPos);
                float diskRadius = (1.0 + (viewDistance / shadowMapFarPlane)) / 25.0;

                float shadow = 0;
                for(int i = 0; i < kSampleOffsetDirections.length(); ++i)
                {
                    float closestDepth = texture(shadowMap,
                                                 fragToLight + kSampleOffsetDirections[i] * diskRadius).r;
                    closestDepth *= shadowMapFarPlane;   // undo mapping [0;1]
                    if(currentDepth - bias > closestDepth)
                    {
                        shadow += 1.0;
                    }
                }
                shadow /= float(kSampleOffsetDirections.length());

                return shadow;
            }

            float shadowFactorPoint_ESM(const vec3 fragmentPos,
                                        const vec3 lightPos,
                                        const samplerCube shadowMap,
                                        const float shadowMapFarPlane,
                                        const float bias,
                                        const float kFactor)
            {
                vec3 fragToLight = fragmentPos - lightPos;
                float currentDepth = (length(fragToLight) / shadowMapFarPlane) - bias;
                float occluder = texture(shadowMap, fragToLight).r;
                float shadow = occluder * exp(-kFactor * currentDepth);
                return clamp(shadow, 0.0, 1.0);
            }

            float shadowFactorPoint_LogESM(const vec3 fragmentPos,
                                           const vec3 lightPos,
                                           const samplerCube shadowMap,
                                           const float shadowMapFarPlane,
                                           const float bias,
                                           const float kFactor)
            {
                vec3 fragToLight = fragmentPos - lightPos;
                float currentDepth = (length(fragToLight) / shadowMapFarPlane) - bias;
                float occluder = texture(shadowMap, fragToLight).r;
                float shadow = exp(occluder - (kFactor * currentDepth));
                return clamp(shadow, 0.0, 1.0);
            }

            vec3 pbrFragColor(const vec3 albedo,
                              const float metallic,
                              const float roughness,
                              const vec3 normal,
                              const float ao,
                              const vec3 ambientLight)
            {
                // TODO: normal mapping

                // calc vectors
                vec3 N = normalize(normal);
                vec3 V = normalize(_viewPosition.xyz - bznkWorldPos.xyz);

                // calculate reflectance at normal incidence;
                // if dia-electric (like plastic) use F0 of 0.04 and
                // if it's a metal, use the albedo color as F0 (metallic workflow)
                vec3 F0 = vec3(0.04);
                F0 = mix(F0, albedo, metallic);

                // reflectance equation...
                vec3 Lo = vec3(0.0);

                // ... for directional lights
                for(uint i = 0U; i < _directionalLightsCount; ++i)
                {
                    vec3 L = -normalize(_directionalLights[i]._direction);
                    vec3 H = normalize(V + L);
                    vec3 radiance = _directionalLights[i]._color;

                    float shadow = 1.0; // "inverted shadow" actually: 0.0 - shadow, 1.0 - light

                    if (0u != _directionalLights[i]._method)
                    {
                        // normal bias (TODO: move it into a vertex shader)

                        float offsetScale = 0;
                        if (0u == _directionalLights[i]._normalBiasMode)
                        {
                            offsetScale = _directionalLights[i]._normalBiasBase;
                        }
                        else if (1u == _directionalLights[i]._normalBiasMode)
                        {
                            offsetScale = max(_directionalLights[i]._normalBiasBase * (1.0 - dot(N, -L)),
                                              _directionalLights[i]._normalBiasMax);
                        }
                        vec3 offsetPos = bznkWorldPos.xyz + N * offsetScale;
                        vec4 lightSpaceFragPos = _directionalLights[i]._viewProjection * vec4(offsetPos, 1.0);

                        // depth bias (TODO: consider using opengl's polygon offset)

                        float depthBias = 0;
                        if (0u == _directionalLights[i]._depthBiasMode)
                        {
                            depthBias = _directionalLights[i]._depthBiasBase;
                        }
                        else if (1u == _directionalLights[i]._depthBiasMode)
                        {
                            depthBias = max(_directionalLights[i]._depthBiasBase * (1.0 - dot(N, -L)),
                                            _directionalLights[i]._depthBiasMax);
                        }

                        // shadow calculation

                        if (1u == _directionalLights[i]._method)
                        {
                            shadow = 1.0 - shadowFactorDirect_Std(lightSpaceFragPos,
                                                                  minireDirectionalLightsShadowMaps[i],
                                                                  depthBias);
                        }
                        else if (2u == _directionalLights[i]._method)
                        {
                            // TODO: consider using opengl's builtin PCF
                            //       (GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE)
                            shadow = 1.0 - shadowFactorDirect_Std_PCF(lightSpaceFragPos,
                                                                      minireDirectionalLightsShadowMaps[i],
                                                                      depthBias);
                        }
                        else if (3u == _directionalLights[i]._method)
                        {
                            shadow = shadowFactorDirect_ESM(lightSpaceFragPos,
                                                            minireDirectionalLightsShadowMaps[i],
                                                            depthBias,
                                                            _directionalLights[i]._methodArg);
                        }
                        else if (4u == _directionalLights[i]._method)
                        {
                            shadow = shadowFactorDirect_LogESM(lightSpaceFragPos,
                                                               minireDirectionalLightsShadowMaps[i],
                                                               depthBias,
                                                               _directionalLights[i]._methodArg);
                        }
                    }

                    shadow = smoothstep(_directionalLights[i]._smoothStepLeft,
                                        _directionalLights[i]._smoothStepRight,
                                        shadow);

                    Lo += shadow * calcLo(N, V, L, H, F0, radiance, albedo, roughness, metallic);
                }

                // ... for point lights
                for(uint i = 0U; i < _pointLightsCount; ++i)
                {
                    // calculate per-light radiance
                    vec3 L = normalize(_pointLights[i]._position - bznkWorldPos).xyz;
                    vec3 H = normalize(V + L);
                    float dist = length(_pointLights[i]._position - bznkWorldPos);
                    float attenuation = 1.0 / (
                        _pointLights[i]._attenuation.x +
                        _pointLights[i]._attenuation.y * dist +
                        _pointLights[i]._attenuation.z * dist * dist
                    );

                    vec3 radiance = _pointLights[i]._color.xyz *
                                    _pointLights[i]._color.w * // TODO: multiply to intensity statically
                                    attenuation;

                    float shadow = 1.0; // "inverted shadow" actually: 0.0 - shadow, 1.0 - light

                    if (0u != _pointLights[i]._method)
                    {
                        // normal bias (TODO: move it into a vertex shader)

                        float offsetScale = 0;
                        if (0u == _pointLights[i]._normalBiasMode)
                        {
                            offsetScale = _pointLights[i]._normalBiasBase;
                        }
                        else if (1u == _pointLights[i]._normalBiasMode)
                        {
                            offsetScale = max(_pointLights[i]._normalBiasBase * (1.0 - dot(N, -L)),
                                              _pointLights[i]._normalBiasMax);
                        }

                        // depth bias (TODO: consider using opengl's polygon offset)

                        float depthBias = 0;
                        if (0u == _pointLights[i]._depthBiasMode)
                        {
                            depthBias = _pointLights[i]._depthBiasBase;
                        }
                        else if (1u == _pointLights[i]._depthBiasMode)
                        {
                            depthBias = max(_pointLights[i]._depthBiasBase * (1.0 - dot(N, -L)),
                                            _pointLights[i]._depthBiasMax);
                        }

                        // shadow calculation

                        if (1u == _pointLights[i]._method)
                        {
                            shadow = 1.0 - shadowFactorPoint_Std(bznkWorldPos.xyz + N * offsetScale,
                                                                 _pointLights[i]._position.xyz,
                                                                 minirePointLightsShadowMaps[i],
                                                                 _pointLights[i]._shadowMapFarPlane,
                                                                 depthBias);
                        }
                        else if (2u == _pointLights[i]._method)
                        {
                            shadow = 1.0 - shadowFactorPoint_Std_PCF(bznkWorldPos.xyz + N * offsetScale,
                                                                     _pointLights[i]._position.xyz,
                                                                     minirePointLightsShadowMaps[i],
                                                                     _pointLights[i]._shadowMapFarPlane,
                                                                     depthBias);
                        }
                        else if (3u == _pointLights[i]._method)
                        {
                            shadow = shadowFactorPoint_ESM(bznkWorldPos.xyz + N * offsetScale,
                                                           _pointLights[i]._position.xyz,
                                                           minirePointLightsShadowMaps[i],
                                                           _pointLights[i]._shadowMapFarPlane,
                                                           depthBias,
                                                           _pointLights[i]._methodArg);
                        }
                        else if (4u == _pointLights[i]._method)
                        {
                            shadow = shadowFactorPoint_LogESM(bznkWorldPos.xyz + N * offsetScale,
                                                              _pointLights[i]._position.xyz,
                                                              minirePointLightsShadowMaps[i],
                                                              _pointLights[i]._shadowMapFarPlane,
                                                              depthBias,
                                                              _pointLights[i]._methodArg);
                        }
                    }

                    shadow = smoothstep(_pointLights[i]._smoothStepLeft,
                                        _pointLights[i]._smoothStepRight,
                                        shadow);

                    Lo += shadow * calcLo(N, V, L, H, F0, radiance, albedo, roughness, metallic);
                }

                // ambient lighting (note that the next IBL tutorial will replace
                // this ambient lighting with environment lighting).
                vec3 ambient = ambientLight * albedo * ao;

                vec3 color = ambient + Lo;

                // HDR tonemapping
                color = color / (color + vec3(1.0));

                // gamma correct
                color = pow(color, vec3(1.0/2.2));

                return color;
            }

            // PBR KIT END //

        )";

        static std::string const kPbrVertShader =
        R"(
            {% include "minire/preamble.incl" %}

            {% include "minire/attributes.incl" %}

            {% if minire_mesh_traits.has_uv %}
            out vec2 bznkFragUv;
            {% endif %}

            {{ minire_assert(minire_mesh_traits.has_normal, "PBR shader reqiures normals") }}
            out vec3 bznkFragNormal;

            {% if minire_mesh_traits.has_tangent %}
            out mat3 bznkTbn;
            {% endif %}

            out vec4 bznkWorldPos;

            flat out uint bznkMeshId;
            flat out vec3 bznkEmissiveFactor;

            {% include "minire/transform.incl" %}
            {% include "minire/ubo.incl" %}

            void main()
            {
                mat4 effectiveModel = minireModelMatrix();

                bznkWorldPos = effectiveModel * vec4(minireVertex, 1.0);
                gl_Position = _viewProjection * bznkWorldPos;

                {% if minire_mesh_traits.has_uv %}
                bznkFragUv = minireUv;
                {% endif %}

                vec3 N = normalize(vec3(effectiveModel * vec4(minireNormal, 0.0)));
                bznkFragNormal = N;

                {% if minire_mesh_traits.has_tangent %}
                vec3 T = normalize(vec3(effectiveModel * vec4(minireTangent, 0.0)));
                T = normalize(T - dot(T, N) * N);
                vec3 B = cross(N, T);
                bznkTbn = mat3(T, B, N);
                {% endif %}

                bznkMeshId = minireMeshId;
                bznkEmissiveFactor = minireEmissiveFactor;
            }
        )";

        static std::string const kPbrFragShader =
        R"(
            {% include "minire/preamble.incl" %}

            // output //

            layout(location = {{ minire_color_output_location }})   out vec3 bznkOutColor;
            layout(location = {{ minire_mesh_id_output_location }}) out uint bznkOutMeshId;

            // input //

            {% if minire_mesh_traits.has_uv %}
            in vec2 bznkFragUv;
            {% endif %}

            {{ minire_assert(minire_mesh_traits.has_normal, "PBR shader reqiures normals") }}
            in vec3 bznkFragNormal;

            {% if minire_mesh_traits.has_tangent %}
            in mat3 bznkTbn;
            {% endif %}

            in vec4 bznkWorldPos;

            flat in uint bznkMeshId;
            flat in vec3 bznkEmissiveFactor;

            // uniforms //

            {% include "minire/ubo.incl" %}

            uniform vec3 bznkAlbedoFactor = vec3(1.0, 1.0, 1.0); {{ minire_register_user_uniform("bznkAlbedoFactor") }}
            {% if minire_extra.kHasAlbedoTexture %}
            uniform sampler2D bznkAlbedoTexture; {{ minire_register_user_uniform("bznkAlbedoTexture") }}
            {% endif %}

            uniform float bznkMetallicFactor = 1.0; {{ minire_register_user_uniform("bznkMetallicFactor") }}
            {% if minire_extra.kHasMetallicTexture %}
            uniform sampler2D bznkMetallicTexture; {{ minire_register_user_uniform("bznkMetallicTexture") }}
            {% endif %}

            uniform float bznkRoughnessFactor = 1.0; {{ minire_register_user_uniform("bznkRoughnessFactor") }}
            {% if minire_extra.kHasRoughnessTexture %}
            uniform sampler2D bznkRoughnessTexture; {{ minire_register_user_uniform("bznkRoughnessTexture") }}
            {% endif %}

            {% if minire_extra.kHasNormalTexture %}
            {{ minire_assert(minire_mesh_traits.has_tangent, "normal textures cannot be used without tangents") }}
            uniform sampler2D bznkNormalTexture; {{ minire_register_user_uniform("bznkNormalTexture") }}
            uniform float bznkNormalScale = 1.0; {{ minire_register_user_uniform("bznkNormalScale") }}
            {% endif %}

            {% if minire_extra.kHasAoTexture %}
            uniform sampler2D bznkAoTexture; {{ minire_register_user_uniform("bznkAoTexture") }}
            {% endif %}
            uniform float bznkAoStrength = 1.0; {{ minire_register_user_uniform("bznkAoStrength") }}

            {% if minire_extra.kHasEmissiveTexture %}
            uniform sampler2D bznkEmissiveTexture; {{ minire_register_user_uniform("bznkEmissiveTexture") }}
            {% endif %}

            {% include "minire/uniforms.incl" %}

            // routines //

            {% include "user/pbr-kit.incl" %}

            // entry pony //

            void main()
            {
                vec3 albedo = bznkAlbedoFactor;
                {% if minire_extra.kHasAlbedoTexture %}
                albedo *= pow(texture(bznkAlbedoTexture, bznkFragUv).rgb, vec3(2.2));
                {% endif %}

                float metallic = bznkMetallicFactor;
                {% if minire_extra.kHasMetallicTexture %}
                metallic *= texture(bznkMetallicTexture, bznkFragUv).{{ minire_extra.kMetallicTexComp }};
                {% endif %}

                float roughness = bznkRoughnessFactor;
                {% if minire_extra.kHasRoughnessTexture %}
                roughness *= texture(bznkRoughnessTexture, bznkFragUv).{{ minire_extra.kRoughnessTexComp }};
                {% endif %}

                {% if minire_extra.kHasNormalTexture and minire_mesh_traits.has_tangent %}
                vec3 normal = normalMapping(
                    bznkTbn,
                    texture(bznkNormalTexture, bznkFragUv).rgb,
                    bznkNormalScale);
                {% else %}
                vec3 normal = bznkFragNormal;
                {% endif %}

                {% if minire_extra.kHasAoTexture %}
                float sampledAo = texture(bznkAoTexture, bznkFragUv).{{ minire_extra.kAoTexComp }};
                float ao = (1.0 + bznkAoStrength * (sampledAo - 1.0));
                {% else %}
                float ao = bznkAoStrength;
                {% endif %}

                vec3 emissiveFactor = bznkEmissiveFactor;
                {% if minire_extra.kHasEmissiveTexture %}
                emissiveFactor *= texture(bznkEmissiveTexture, bznkFragUv).rgb;
                {% endif %}

                bznkOutColor = pbrFragColor(albedo,
                                            metallic,
                                            roughness,
                                            normal,
                                            ao,
                                            minireAmbientLight);
                bznkOutColor += emissiveFactor;

                bznkOutMeshId = bznkMeshId;
            }
        )";
    }

    Pbr::UniformIndeces::UniformIndeces(material::UniformNames const & names)
    {
        for(size_t i = 0; i < names.size(); ++i)
        {
            std::string const & name = names[i];

            if ("bznkAlbedoFactor" == name)
            {
                _albedoFactor = i;
            }
            else if ("bznkAlbedoTexture" == name)
            {
                _albedoTexture = i;
            }
            else if ("bznkMetallicFactor" == name)
            {
                _metallicFactor = i;
            }
            else if ("bznkMetallicTexture" == name)
            {
                _metallicTexture = i;
            }
            else if ("bznkRoughnessFactor" == name)
            {
                _roughnessFactor = i;
            }
            else if ("bznkRoughnessTexture" == name)
            {
                _roughnessTexture = i;
            }
            else if ("bznkNormalTexture" == name)
            {
                _normalTexture = i;
            }
            else if ("bznkNormalScale" == name)
            {
                _normalScale = i;
            }
            else if ("bznkAoStrength" == name)
            {
                _aoStrength = i;
            }
            else if ("bznkAoTexture" == name)
            {
                _aoTexture = i;
            }
            else if ("bznkEmissiveTexture" == name)
            {
                _emissiveTexture = i;
            }
            else
            {
                MINIRE_THROW("unexpected user uniform: \"{}\", uniformNames: {}",
                             name, fmt::join(names, ", "));
            }
        }
    }

    // NOTE 1: any changes in _model must increase _revision!
    // NOTE 2: value of *TextureComponent are immuable,
    //         because they require shaders re-compilation.

    Pbr::Pbr(models::PbrMaterial const & model)
        : _model(model)
        , _modelInvalidated(true)
    {}

    material::Program Pbr::render() const
    {
        // prepare extra parameters
        nlohmann::json extra
        {
            {"kHasAlbedoTexture",   _model._albedoTexture.has_value()},
            {"kHasMetallicTexture", _model._metallicTexture.has_value()},
            {"kHasRoughnessTexture",_model._roughnessTexture.has_value()},
            {"kHasNormalTexture",   _model._normalTexture.has_value()},
            {"kHasAoTexture",       _model._aoTexture.has_value()},
            {"kHasEmissiveTexture", _model._emissiveTexture.has_value()},
            {"kMetallicTexComp",    toString(_model._metallicTextureComponent)},
            {"kRoughnessTexComp",   toString(_model._roughnessTextureComponent)},
            {"kAoTexComp",          toString(_model._aoTextureComponent)},
        };

        // prepare shaders templates
        material::Shaders shaders;
        shaders[static_cast<int>(material::ShaderType::kVertex)] = kPbrVertShader;
        shaders[static_cast<int>(material::ShaderType::kFragment)] = kPbrFragShader;

        return material::Program
        {
            ._shaders = std::move(shaders),
            ._extra = std::make_unique<nlohmann::json>(std::move(extra)),
            ._includes = {{"pbr-kit.incl", kPbrKit}},
        };
    }

    material::UniformValues const &
    Pbr::updateUserUniforms(material::UniformNames const & uniformNames,
                            models::TextureResolver const & textureResolver) const
    {
        // memoize uniform indeces
        if (!_indeces)
        {
            _indeces.emplace(uniformNames);
            _modelInvalidated = true;
        }

        // setup uniform values
        if (_modelInvalidated)
        {
            _values.resize(uniformNames.size());
            assert(_indeces);

            assert(_indeces->_albedoFactor < _values.size());
            _values[_indeces->_albedoFactor] = _model._albedoFactor;
            if (_model._albedoTexture)
            {
                if (!_albedoTextureHandle)
                {
                    _albedoTextureHandle = textureResolver.resolve(*_model._albedoTexture,
                                                                   _model._albedoSampler);
                }

                assert(_indeces->_albedoTexture < _values.size());
                _values[_indeces->_albedoTexture] = _albedoTextureHandle;
            }

            assert(_indeces->_metallicFactor < _values.size());
            _values[_indeces->_metallicFactor] = _model._metallicFactor;
            if (_model._metallicTexture)
            {
                if (!_metallicTextureHandle)
                {
                    _metallicTextureHandle = textureResolver.resolve(*_model._metallicTexture,
                                                                     _model._metallicSampler);
                }

                assert(_indeces->_metallicTexture < _values.size());
                _values[_indeces->_metallicTexture] = _metallicTextureHandle;
            }

            assert(_indeces->_roughnessFactor < _values.size());
            _values[_indeces->_roughnessFactor] = _model._roughnessFactor;
            if (_model._roughnessTexture)
            {
                if (!_roughnessTextureHandle)
                {
                    _roughnessTextureHandle = textureResolver.resolve(*_model._roughnessTexture,
                                                                      _model._roughnessSampler);
                }

                assert(_indeces->_roughnessTexture < _values.size());
                _values[_indeces->_roughnessTexture] = _roughnessTextureHandle;
            }

            if (_model._normalTexture)
            {
                if (!_normalTextureHandle)
                {
                    _normalTextureHandle = textureResolver.resolve(*_model._normalTexture,
                                                                   _model._normalSampler);
                }

                assert(_indeces->_normalTexture < _values.size());
                _values[_indeces->_normalTexture] = _normalTextureHandle;

                assert(_indeces->_normalScale < _values.size());
                _values[_indeces->_normalScale] = _model._normalScale;
            }

            assert(_indeces->_aoStrength < _values.size());
            _values[_indeces->_aoStrength] = _model._aoStrength;
            if (_model._aoTexture)
            {
                if (!_aoTextureHandle)
                {
                    _aoTextureHandle = textureResolver.resolve(*_model._aoTexture,
                                                               _model._aoSampler);
                }

                assert(_indeces->_aoTexture < _values.size());
                _values[_indeces->_aoTexture] = _aoTextureHandle;
            }

            if (_model._emissiveTexture)
            {
                if (!_emissiveTextureHandle)
                {
                    _emissiveTextureHandle = textureResolver.resolve(*_model._emissiveTexture,
                                                                     _model._emissiveSampler);
                }

                assert(_indeces->_emissiveTexture < _values.size()); 
                _values[_indeces->_emissiveTexture] = _emissiveTextureHandle;
            }

            _modelInvalidated = false;
        }

        assert (_values.size() == uniformNames.size());
        return _values;
    }

    std::string Pbr::slugImpl() const
    {
        std::string result;

        result += _model._albedoTexture ? "A:TF/" : "A:F/";

        result += _model._metallicTexture
            ? fmt::format("M:T{}F/", toString(_model._metallicTextureComponent))
            : std::string("M:F/");

        result += _model._roughnessTexture
            ? fmt::format("R:T{}F/", toString(_model._roughnessTextureComponent))
            : std::string("R:F/");

        result += _model._normalTexture ? "N:T/" : "N:0/";

        result += _model._aoTexture
            ? fmt::format("O:T{}/", toString(_model._aoTextureComponent))
            : std::string("O:0/");

        result += _model._emissiveTexture ? "E:T/" : "E:0/";

        result += "!";
        return result;
    }
}
