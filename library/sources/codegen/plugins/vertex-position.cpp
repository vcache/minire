#include <codegen/plugins/vertex-position.hpp>

#include <minire/errors.hpp>

namespace minire::codegen::plugins
{
    std::vector<std::string> const & VertexPosition::uniforms() const
    {
        static std::vector<std::string> const kUniforms;
        return kUniforms;
    }

    // TODO: add normals and tangents here, and rename to BasicAttribs
    void VertexPosition::setup(codegen::Traits const & traits,
                               inja::Environment & env,
                               nlohmann::json & vars,
                               opengl::Program::AttribLocations & attribs) const
    {
        env.include_template("shaders/vertex-position.incl",
                             env.parse("in vec3 {{ bznkVertex }};\n"));

        {
            auto [_, inserted] = vars.emplace("bznkVertex", "bznkVertex");
            MINIRE_INVARIANT(inserted, "key already exists: bznkVertex");
        }

        {
            assert(traits._attribLocations._vertexAttribute >= 0);
            auto [_, inserted] = attribs.emplace("bznkVertex",
                                                 traits._attribLocations._vertexAttribute);
            MINIRE_INVARIANT(inserted, "failed to insert bznkVertex attrib");
        }
    }

}