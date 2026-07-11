#include <rasterizer/materials/templates.hpp>

#include <rasterizer/ubo.hpp>

namespace minire::rasterizer::materials
{
    std::string const & preambleTemplate()
    {
        static std::string const kValue =
        R"(
            // Shader type: {{ minire_shader_type }}
            // Material name: {{ minire_material_name }}
            // Material slug: {{ minire_material_slug }}

            #version 430 core
        )";
        return kValue;
    }

    std::string const & attributesTemplate()
    {
        static std::string const kValue =
        R"(
            {{ minire_assert(minire_shader_type == "vertex", "attributes not in a vertex shader") }}

            layout(location = {{ minire_vertex_attrib_location }}) in vec3 minireVertex;

            {% if minire_mesh_traits.has_uv %}
            layout(location = {{ minire_uv_attrib_location }}) in vec2 minireUv;
            {% endif %}

            {% if minire_mesh_traits.has_normal %}
            layout(location = {{ minire_normal_attrib_location }}) in vec3 minireNormal;
            {% endif %}

            {% if minire_mesh_traits.has_tangent %}
            layout(location = {{ minire_tangent_attrib_location }}) in vec3 minireTangent;
            {% endif %}

            {% if minire_mesh_traits.has_skin and not minire_is_guard_set("skinning-attribs") %}
            layout(location = {{ minire_joints_attrib_location }}) in uvec4 minireJoints;
            layout(location = {{ minire_weights_attrib_location }}) in vec4 minireWeights;
            {{ minire_set_guard("skinning-attribs") }}
            {% endif %}

            // NOTE: these attributes are supposed to have uniform-like behaviour,
            //       i.e. have the same value across the whole mesh, therefore,
            //       they must be passed to other shaders (fragment, geometry, etc)
            //       throught `flat` out/in out variables to prevent their interpolation.
            //
            //       For example. In vertex shader:
            //
            //          flat out uint meshId;
            //          ...
            //          void main() { meshId = minireMeshId; }
            //
            //       In fragment shader:
            //
            //          flat in uint meshId;
            //          ...

            layout(location = {{ minire_instanced_emissive_factor_attrib_location }})
            in vec3 minireEmissiveFactor;

            layout(location = {{ minire_instanced_mesh_id_attrib_location }})
            in uint minireMeshId;
        )";
        return kValue;
    }

    std::string const & transformTemplate()
    {
        static std::string const kValue =
        R"(
            {{ minire_assert(minire_shader_type == "vertex", "transform not in a vertex shader") }}

            {% if minire_mesh_traits.has_skin %}

                {% if not minire_is_guard_set("skinning-attribs") %}
                    layout(location = {{ minire_joints_attrib_location }}) in uvec4 minireJoints;
                    layout(location = {{ minire_weights_attrib_location }}) in vec4 minireWeights;
                    {{ minire_set_guard("skinning-attribs") }}
                {% endif %}

                layout(std430, binding = {{ minire_skinning_ssbo_binding_point }})
                readonly buffer MinireSkinningVectors
                {
                    vec4 minireBones[];
                };

                mat4 minireBoneMatrix(uint boneIndex)
                {
                    uint offset = (uint(gl_InstanceID) * {{ minire_max_bones }}u + boneIndex) * 3u;

                    vec4 row0 = minireBones[offset + 0u];
                    vec4 row1 = minireBones[offset + 1u];
                    vec4 row2 = minireBones[offset + 2u];

                    return mat4
                    (
                        vec4(row0.x, row1.x, row2.x, 0.0), // Column 0 (Right)
                        vec4(row0.y, row1.y, row2.y, 0.0), // Column 1 (Up)
                        vec4(row0.z, row1.z, row2.z, 0.0), // Column 2 (Forward)
                        vec4(row0.w, row1.w, row2.w, 1.0)  // Column 3 (Translation)
                    );
                }

                mat4 minireModelMatrix()
                {
                    return minireWeights.x * minireBoneMatrix(minireJoints.x)
                         + minireWeights.y * minireBoneMatrix(minireJoints.y)
                         + minireWeights.z * minireBoneMatrix(minireJoints.z)
                         + minireWeights.w * minireBoneMatrix(minireJoints.w);
                }

            {% else %} {# minire_mesh_traits.has_skin #}

                layout(location = {{ minire_instanced_model_attrib_location }})
                in mat4 minireModel;

                mat4 minireModelMatrix()
                {
                    return minireModel;
                }

            {% endif %}
        )";
        return kValue;
    }

    std::string const & uniformsTemplate()
    {
        static std::string const kValue =
        R"(
            uniform sampler2D minireDirectionalLightsShadowMaps[{{ minire_max_directional_lights }}];
            {{ minire_set_directional_lights_shadow_maps_uniform_name("minireDirectionalLightsShadowMaps") }}

            uniform samplerCube minirePointLightsShadowMaps[{{ minire_max_points_lights }}];
            {{ minire_set_point_lights_shadow_maps_uniform_name("minirePointLightsShadowMaps") }}

            uniform vec3 minireAmbientLight = vec3(0.03);
            {{ minire_set_ambient_light_uniform_name("minireAmbientLight") }}
        )";
        return kValue;
    }

    std::string const & uboTemplate()
    {
        static std::string const kValue = []
        {
            std::string result = Ubo::interfaceBlock();
            result += "{{ minire_set_ubo_name(\"BznkDatablock\") }}";
            return result;
        }();
        return kValue;
    }
}