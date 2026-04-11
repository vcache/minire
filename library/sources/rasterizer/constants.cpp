#include <rasterizer/constants.hpp>

namespace minire::rasterizer
{
    // NOTE: these shaders are based on https://learnopengl.com/

    std::string Constants::kPbrKit = R"(

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
                                                              bznkDirectionalLightsShadowMaps[i],
                                                              depthBias);
                    }
                    else if (2u == _directionalLights[i]._method)
                    {
                        // TODO: consider using opengl's builtin PCF
                        //       (GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE)
                        shadow = 1.0 - shadowFactorDirect_Std_PCF(lightSpaceFragPos,
                                                                  bznkDirectionalLightsShadowMaps[i],
                                                                  depthBias);
                    }
                    else if (3u == _directionalLights[i]._method)
                    {
                        shadow = shadowFactorDirect_ESM(lightSpaceFragPos,
                                                        bznkDirectionalLightsShadowMaps[i],
                                                        depthBias,
                                                        _directionalLights[i]._methodArg);
                    }
                    else if (4u == _directionalLights[i]._method)
                    {
                        shadow = shadowFactorDirect_LogESM(lightSpaceFragPos,
                                                           bznkDirectionalLightsShadowMaps[i],
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
                                                             bznkPointLightsShadowMaps[i],
                                                             _pointLights[i]._shadowMapFarPlane,
                                                             depthBias);
                    }
                    else if (2u == _pointLights[i]._method)
                    {
                        shadow = 1.0 - shadowFactorPoint_Std_PCF(bznkWorldPos.xyz + N * offsetScale,
                                                                 _pointLights[i]._position.xyz,
                                                                 bznkPointLightsShadowMaps[i],
                                                                 _pointLights[i]._shadowMapFarPlane,
                                                                 depthBias);
                    }
                    else if (3u == _pointLights[i]._method)
                    {
                        shadow = shadowFactorPoint_ESM(bznkWorldPos.xyz + N * offsetScale,
                                                       _pointLights[i]._position.xyz,
                                                       bznkPointLightsShadowMaps[i],
                                                       _pointLights[i]._shadowMapFarPlane,
                                                       depthBias,
                                                       _pointLights[i]._methodArg);
                    }
                    else if (4u == _pointLights[i]._method)
                    {
                        shadow = shadowFactorPoint_LogESM(bznkWorldPos.xyz + N * offsetScale,
                                                          _pointLights[i]._position.xyz,
                                                          bznkPointLightsShadowMaps[i],
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

    std::string Constants::kModelSkinningKit = R"(
        {% if kHasSkins %}
        in uvec4 bznkJoints;
        in vec4 bznkWeights;

        // pre-multiplied: globalTransform * inverseBindMatrix
        uniform mat4 bznkBones[{{ kMaxBones }}];

        mat4 getEffectiveModelMatrix()
        {
            return bznkWeights.x * bznkBones[bznkJoints.x]
                 + bznkWeights.y * bznkBones[bznkJoints.y]
                 + bznkWeights.z * bznkBones[bznkJoints.z]
                 + bznkWeights.w * bznkBones[bznkJoints.w];
        }
        {% else %}
        uniform mat4 bznkModel;
        mat4 getEffectiveModelMatrix()
        {
            return bznkModel;
        }
        {% endif %}
    )";

    std::string Constants::kPbrVertShader = R"(
        #version 330 core

        in vec3 bznkVertex;

        {% if kHasUvs %}
        in vec2 bznkUv;
        out vec2 bznkFragUv;
        {% endif %}

        in vec3 bznkNormal;
        out vec3 bznkFragNormal;

        {% if kHasTangents %}
        in vec3 bznkTangent;
        out mat3 bznkTbn;
        {% endif %}

        {% include "shaders/model-skinning-kit.incl" %}

        out vec4 bznkWorldPos;

        {{ kUboDatablock }}

        void main()
        {
            mat4 effectiveModel = getEffectiveModelMatrix();

            bznkWorldPos = effectiveModel * vec4(bznkVertex, 1.0);
            gl_Position = _viewProjection * bznkWorldPos;

            {% if kHasUvs %}
            bznkFragUv = bznkUv;
            {% endif %}

            vec3 N = normalize(vec3(effectiveModel * vec4(bznkNormal, 0.0)));
            bznkFragNormal = N;

            {% if kHasTangents %}
            vec3 T = normalize(vec3(effectiveModel * vec4(bznkTangent, 0.0)));
            T = normalize(T - dot(T, N) * N);
            vec3 B = cross(N, T);
            bznkTbn = mat3(T, B, N);
            {% endif %}
        }
    )";

    std::string Constants::kPbrFragShader = R"(
        #version 330 core

        // output //

        layout(location = 0) out vec3 bznkOutColor;
        layout(location = 1) out uint bznkOutMeshId;

        // input //

        {% if kHasUvs %}
        in vec2 bznkFragUv;
        {% endif %}

        in vec3 bznkFragNormal;

        {% if kHasTangents %}
        in mat3 bznkTbn;
        {% endif %}

        in vec4 bznkWorldPos;

        // uniforms //

        {{ kUboDatablock }}

        uniform vec3 bznkAlbedoFactor = vec3(1.0, 1.0, 1.0);
        {% if kHasAlbedoTexture %}
        uniform sampler2D bznkAlbedoTexture;
        {% endif %}

        uniform float bznkMetallicFactor = 1.0;
        {% if kHasMetallicTexture %}
        uniform sampler2D bznkMetallicTexture;
        {% endif %}

        uniform float bznkRoughnessFactor = 1.0;
        {% if kHasRoughnessTexture %}
        uniform sampler2D bznkRoughnessTexture;
        {% endif %}

        {% if kHasNormalTexture %}
        uniform sampler2D bznkNormalTexture;
        uniform float bznkNormalScale = 1.0;
        {% endif %}

        {% if kHasAoTexture %}
        uniform sampler2D bznkAoTexture;
        {% endif %}
        uniform float bznkAoStrength = 1.0;

        {% if kHasEmissiveTexture %}
        uniform sampler2D bznkEmissiveTexture;
        {% endif %}

        uniform sampler2D bznkDirectionalLightsShadowMaps[{{ kMaxDirectionalLights }}];
        uniform samplerCube bznkPointLightsShadowMaps[{{ kMaxPointLights }}];

        uniform vec3 bznkEmissiveFactor = vec3(0.0, 0.0, 0.0);

        uniform vec3 bznkAmbientLight = vec3(0.03);

        // TODO: make it optional
        uniform uint bznkMeshId = 0u;

        // routines //

        {% include "shaders/pbr-kit.incl" %}

        // entry pony //

        void main()
        {
            vec3 albedo = bznkAlbedoFactor;
            {% if kHasAlbedoTexture %}
            albedo *= pow(texture(bznkAlbedoTexture, bznkFragUv).rgb, vec3(2.2));
            {% endif %}

            float metallic = bznkMetallicFactor;
            {% if kHasMetallicTexture %}
            metallic *= texture(bznkMetallicTexture, bznkFragUv).{{ kMetallicTexComp }};
            {% endif %}

            float roughness = bznkRoughnessFactor;
            {% if kHasRoughnessTexture %}
            roughness *= texture(bznkRoughnessTexture, bznkFragUv).{{ kRoughnessTexComp }};
            {% endif %}

            {% if kHasNormalTexture and kHasTangents %}
            vec3 normal = normalMapping(
                bznkTbn,
                texture(bznkNormalTexture, bznkFragUv).rgb,
                bznkNormalScale);
            {% else %}
            vec3 normal = bznkFragNormal;
            {% endif %}

            {% if kHasAoTexture %}
            float sampledAo = texture(bznkAoTexture, bznkFragUv).{{ kAoTexComp }};
            float ao = (1.0 + bznkAoStrength * (sampledAo - 1.0));
            {% else %}
            float ao = bznkAoStrength;
            {% endif %}

            vec3 emissiveFactor = bznkEmissiveFactor;
            {% if kHasEmissiveTexture %}
            emissiveFactor *= texture(bznkEmissiveTexture, bznkFragUv).rgb;
            {% endif %}

            bznkOutColor = pbrFragColor(albedo,
                                        metallic,
                                        roughness,
                                        normal,
                                        ao,
                                        bznkAmbientLight);
            bznkOutColor += emissiveFactor;

            bznkOutMeshId = bznkMeshId;
        }
    )";
}
