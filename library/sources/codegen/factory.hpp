#pragma once

#include <codegen/names-coder.hpp>
#include <codegen/traits.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>

#include <inja/inja.hpp>
#include <minire/errors.hpp>

#include <cassert>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace minire::codegen
{
    template<typename... Plugins>
    class Factory
        : public Plugins...
    {
    public:
        class Program
            : public Plugins::Instance<Program>...
        {
        public:
            explicit Program(NamesCoder const & uniformCoder,
                             Traits const & traits,
                             opengl::Program && program)
                : Plugins::Instance<Program>(uniformCoder, traits)...
                , _program(std::move(program))
            {
                _uniformCodeToId.resize(uniformCoder.size(), -1);
                for(size_t i = 0; i < uniformCoder.size(); ++i)
                {
                    _uniformCodeToId[i] = _program.getUniformLocation(uniformCoder[i].c_str());
                }
            }

            template<typename T>
            void setUniformByCode(size_t const uniformCode, T && value) const
            {
                assert(uniformCode < _uniformCodeToId.size());
                assert(_uniformCodeToId[uniformCode] >= 0);
                _program.setUniform(_uniformCodeToId[uniformCode],
                                    std::forward<T>(value));
            }

            void use() const { _program.use(); }

        private:
            std::vector<GLint> _uniformCodeToId;
            opengl::Program    _program;
        };

    public:
        size_t getUniformCode(std::string const & name) const
        {
            return _uniformCoder.find(name);
        }

    public:
        using Shaders = std::unordered_map<GLenum, std::string>;
        using Uniforms = std::unordered_set<std::string>;

        explicit Factory(Shaders shaders,
                         Uniforms uniforms = {})
            : _shaders(std::move(shaders))
            , _uniforms(std::move(uniforms))
        {
            for(std::string const & uniform : _uniforms)
            {
                _uniformCoder.getOrMakeUniformCode(uniform);
            }

            ([&]
            {
                for(std::string const & uniform : Plugins::uniforms())
                {
                    _uniformCoder.getOrMakeUniformCode(uniform);
                }
            }(), ...);
        }

        Program build(Traits const & traits) const
        {
            inja::Environment env;
            nlohmann::json vars;
            opengl::Program::AttribLocations attribLocations;

            // update env and vars
            ([&] { Plugins::setup(traits, env, vars, attribLocations); }(), ...);

            // compile the shaders
            std::vector<opengl::Shader::Sptr> shaders;
            shaders.reserve(_shaders.size());
            for(auto const & [type, source] : _shaders)
            {
                MINIRE_INVARIANT(!source.empty(), "empty shader of type {}", type);
                std::string renderedSource = env.render(source, vars);
                shaders.push_back(std::make_shared<opengl::Shader>(type, renderedSource));
            }

            // build the program
            opengl::Program program(shaders, attribLocations);

            // build the result
            return Program(_uniformCoder, traits, std::move(program));
        }

        size_t getOrMakeUniformCode(std::string const & name)
        {
            return _uniformCoder.getOrMakeUniformCode(name);
        }

    private:
        Shaders    _shaders;
        Uniforms   _uniforms;
        NamesCoder _uniformCoder;
    };

    template<typename... Plugins>
    class CachedFactory
        : public Factory<Plugins...>
    {
        using Base = Factory<Plugins...>;

    public:
        using Base::Base;

        // Find cached program (or creates a new one) and use() it
        auto const & getUsingProgram(Traits const & traits) const
        {
            // lookup in the cache
            auto it = _programCache.find(traits);
            if (it == _programCache.cend())
            {
                // build a new program and store her into cache
                auto [nit, inserted] = _programCache.emplace(traits, Base::build(traits));
                MINIRE_INVARIANT(inserted, "failed to save new program");
                it = nit;
            }

            // finish
            assert(it != _programCache.cend());
            it->second.use();
            return it->second;
        }

    private:
        using ProgramCache = std::unordered_map<Traits,
                                                typename Base::Program>;

        mutable ProgramCache _programCache;
    };
}
