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

            #version 330 core
        )";
        return kValue;
    }

    std::string const & attributesTemplate()
    {
        static std::string const kValue =
        R"(
            {{ minire_assert(minire_shader_type == "vertex", "attributes not in a vertex shader") }}

            in vec3 minireVertex; {{ minire_set_vertex_attrib_name("minireVertex") }}

            {% if minire_mesh_traits.has_uv %}
            in vec2 minireUv; {{ minire_set_uv_attrib_name("minireUv") }}
            {% endif %}

            {% if minire_mesh_traits.has_normal %}
            in vec3 minireNormal; {{ minire_set_normal_attrib_name("minireNormal") }}
            {% endif %}

            {% if minire_mesh_traits.has_tangent %}
            in vec3 minireTangent; {{ minire_set_tangent_attrib_name("minireTangent") }}
            {% endif %}

            {% if minire_mesh_traits.has_skin and not minire_is_guard_set("skinning-attribs") %}
            in uvec4 minireJoints; {{ minire_set_joints_attrib_name("minireJoints") }}
            in vec4 minireWeights; {{ minire_set_weights_attrib_name("minireWeights") }}
            {{ minire_set_guard("skinning-attribs") }}
            {% endif %}
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
            in uvec4 minireJoints; {{ minire_set_joints_attrib_name("minireJoints") }}
            in vec4 minireWeights; {{ minire_set_weights_attrib_name("minireWeights") }}
            {{ minire_set_guard("skinning-attribs") }}
            {% endif %}

            // pre-multiplied: globalTransform * inverseBindMatrix
            uniform mat4 minireBones[{{ minire_max_bones }}];
            {{ minire_set_bones_uniform_name("minireBones") }}

            mat4 minireModelMatrix()
            {
                return minireWeights.x * minireBones[minireJoints.x]
                     + minireWeights.y * minireBones[minireJoints.y]
                     + minireWeights.z * minireBones[minireJoints.z]
                     + minireWeights.w * minireBones[minireJoints.w];
            }

            {% else %}

            uniform mat4 minireModel; {{ minire_set_model_uniform_name("minireModel") }}
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

            uniform vec3 minireEmissiveFactor = vec3(0.0, 0.0, 0.0);
            {{ minire_set_emissive_factor_uniform_name("minireEmissiveFactor") }}

            uniform vec3 minireAmbientLight = vec3(0.03);
            {{ minire_set_ambient_light_uniform_name("minireAmbientLight") }}

            uniform uint minireMeshId = 0u;
            {{ minire_set_mesh_id_uniform_name("minireMeshId") }}
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