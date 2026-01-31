#pragma once

#include <opengl/program.hpp>
#include <opengl/shader.hpp>

#include <minire/errors.hpp>

#include <algorithm>
#include <cassert>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace minire::opengl
{
    template<typename ProgramKey,
             typename ProgramKeyHash = std::hash<ProgramKey>>
    class ProgramCache
    {
    public:
        explicit ProgramCache(std::unordered_set<size_t> uniformsCodes)
            : _uniformsCodes(std::move(uniformsCodes))
            , _maxUniformCode([this]() -> size_t // NOTE: the order of _uniformsCodes and _maxUniformCode
            {
                if (_uniformsCodes.empty()) return 0;
                auto it = std::ranges::max_element(_uniformsCodes);
                assert(it != _uniformsCodes.cend());
                return *it;
            }())
        {}

        virtual ~ProgramCache() = default;

    public:
        struct ProgramData
        {
            Program            _program;
            std::vector<GLint> _uniforms;

            template<typename T>
            void setUniformByCode(size_t code, T const & value) const
            {
                assert(code < _uniforms.size());
                assert(_uniforms[code] >= 0);
                _program.setUniform(_uniforms[code], value);
            }
        };

        // Find cached program (or creates a new one) and use() it
        ProgramData const & getUsingProgram(ProgramKey const & programKey) const
        {
            // lookup in the cache
            auto it = _programCache.find(programKey);
            if (it == _programCache.cend())
            {
                // render shaders
                Shaders shadersData = renderShaders(programKey);
                MINIRE_INVARIANT(!shadersData._sources.empty(), "no shader sources provided");

                // compile the shaders
                std::vector<Shader::Sptr> shaders;
                shaders.reserve(shadersData._sources.size());
                for(auto const & [type, source] : shadersData._sources)
                {
                    MINIRE_INVARIANT(!source.empty(), "empty shader of type {}", type);
                    shaders.push_back(std::make_shared<Shader>(type, source));
                }

                // build a new program and store her into cache
                auto [nit, inserted] = _programCache.emplace(programKey, ProgramData
                {
                    ._program = Program(std::move(shaders), shadersData._attribLocations),
                    ._uniforms = std::vector<GLint>(_maxUniformCode + 1, -1),
                });
                MINIRE_INVARIANT(inserted, "failed to save new program");
                it = nit;

                // find uniform locations
                ProgramData & programData = it->second;
                for(size_t const uniformCode : shadersData._uniformCodes)
                {
                    assert(_uniformsCodes.contains(uniformCode));
                    assert(uniformCode <= _maxUniformCode);
                    assert(uniformCode <= programData._uniforms.size());
                    std::string const & uniformName = getUniformName(uniformCode);
                    programData._uniforms[uniformCode] =
                        programData._program.getUniformLocation(uniformName.c_str());
                }
            }

            // finish
            assert(it != _programCache.cend());
            it->second._program.use();
            return it->second;
        }

    protected:
        struct Shaders
        {
            using Sources = std::unordered_map<GLenum, std::string>;
            using UniformCodes = std::unordered_set<size_t>;
            using AttribLocations = Program::AttribLocations;

            Sources         _sources;
            UniformCodes    _uniformCodes;
            AttribLocations _attribLocations;
        };

        virtual Shaders renderShaders(ProgramKey const &) const = 0;
        virtual std::string getUniformName(size_t const) const = 0;

    private:
        using Store = std::unordered_map<ProgramKey, ProgramData, ProgramKeyHash>;

        mutable Store _programCache;

        std::unordered_set<size_t> const _uniformsCodes;
        size_t const                     _maxUniformCode;
    };
}