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
        
        vec3 pbrFragColor(const vec3 albedo,
                          const float metallic,
                          const float roughness,
                          const vec3 normal,
                          const float ao)
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

            // reflectance equation
            vec3 Lo = vec3(0.0);
            for(uint i = 0U; i < _lightsCount; ++i) 
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
                Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            }   
            
            // ambient lighting (note that the next IBL tutorial will replace 
            // this ambient lighting with environment lighting).
            vec3 ambient = vec3(0.03) * albedo * ao;

            vec3 color = ambient + Lo;

            // HDR tonemapping
            color = color / (color + vec3(1.0));

            // gamma correct
            color = pow(color, vec3(1.0/2.2)); 

            return color;
        }

        // PBR KIT END //

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

            vec3 N = normalize(vec3(bznkModel * vec4(bznkNormal, 0.0)));
            bznkFragNormal = N;

            {% if kHasTangents %}
            vec3 T = normalize(vec3(bznkModel * vec4(bznkTangent, 0.0)));
            T = normalize(T - dot(T, N) * N);
            vec3 B = cross(N, T);
            bznkTbn = mat3(T, B, N);
            {% endif %}
        }
    )";

    std::string Constants::kPbrFragShader = R"(
        #version 330 core

        // output //

        out vec3 bznkOutColor;

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

        uniform vec3 bznkEmissiveFactor = vec3(0.0, 0.0, 0.0);

        uniform float bznkColorFactor = 1.0;

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
                                        ao);
            bznkOutColor += emissiveFactor;
            bznkOutColor *= bznkColorFactor;
        }
    )";
}
