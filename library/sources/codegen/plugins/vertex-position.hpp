#pragma once

#include <codegen/factory.hpp>

namespace minire::codegen::plugins
{
    class VertexPosition
    {
    public:
        template<typename Base>
        class Instance
        {
        public:
            explicit Instance(codegen::NamesCoder const &,
                              codegen::Traits const &)
            {}
        };

    public:
        std::vector<std::string> const & uniforms() const;

        void setup(codegen::Traits const & traits,
                   inja::Environment &,
                   nlohmann::json &,
                   opengl::Program::AttribLocations & attribs) const;
    };
}