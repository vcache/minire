#pragma once

#include <minire/errors.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/models/texture-handle.hpp>
#include <minire/utils/demangle.hpp>

#include <fmt/format.h>
#include <glm/ext/matrix_double2x2.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <nlohmann/json_fwd.hpp>

#include <array>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace minire
{
    namespace material
    {
        enum class ShaderType
        {
            kVertex, kFragment, kGeometry, kTessControl, kTessEvaluation, kCompute,
            __kCount__
        };

        std::string_view toString(ShaderType);

        using Shaders = std::array<std::string, static_cast<size_t>(ShaderType::__kCount__)>;
        using JsonUptr = std::unique_ptr<nlohmann::json>;
        using Includes = std::unordered_map<std::string, std::string>;

        struct Program
        {
            Shaders  _shaders;  // jinja-template of program's shaders
            JsonUptr _extra;    // extra args for templates, can be nullptr
            Includes _includes; // name -> jinja-template
        };

        using UniformValue = std::variant
        <
            // value isn't set
            std::monostate,

            // scalars
            bool, int32_t, uint32_t, float, double,

            // vectors
            glm::vec2,    glm::vec3,    glm::vec4,
            glm::dvec2,   glm::dvec3,   glm::dvec4,
            glm::ivec2,   glm::ivec3,   glm::ivec4,
            glm::uvec2,   glm::uvec3,   glm::uvec4,
            glm::bvec2,   glm::bvec3,   glm::bvec4,

            // matrices
            glm::mat2,    glm::mat3,    glm::mat4,
            glm::dmat2,   glm::dmat3,   glm::dmat4,

            glm::mat2x3,  glm::mat2x4,  glm::mat3x2,
            glm::mat3x4,  glm::mat4x2,  glm::mat4x3,

            glm::dmat2x3, glm::dmat2x4, glm::dmat3x2,
            glm::dmat3x4, glm::dmat4x2, glm::dmat4x3,

            // special values
            models::TextureHandle::Sptr,

            // TODO: this is a hack for cube-shadow-map.cpp, should re-desgin this
            //       in a more elegant way
            std::array<glm::mat4, 6>
        >;

        using UniformNames = std::vector<std::string>;
        using UniformValues = std::vector<material::UniformValue>;
    }

    // This is an instance of a given Material.
    class Material
    {
    public:
        using Sptr = std::shared_ptr<Material>;
        using Wptr = std::weak_ptr<Material>;

        Material() = default;
        virtual ~Material() = default;

    public:
        // The function must be deterministic, that is, for the same set of inputs
        // it MUST return the same Program, so that it can be memoized.
        // The rendered Program will be cached using 2-level cache key:
        //      cache[material.slug()][meshFeatures]
        // When all three matches, minire will consider Programs the same
        virtual material::Program render() const = 0;

        // Will be called each time before material usage.
        // It is guaranteed that the input list, it's contents (including ordering),
        // is immutable, i.e. it is safe to store indeces inside this vector.
        // The list of uniforms is auto-collected by minire_register_user_uniform.
        // Implemetation is responsible for keeping lifetime of result.
        virtual material::UniformValues const & updateUserUniforms(material::UniformNames const &,
                                                                   models::TextureResolver const &) const = 0;

        // Can contain anything, only for debug purposes, non-identiying
        virtual std::string humanReadableName() const
        {
            return boost::core::demangle(typeid(*this).name());
        }

        // Must be the same for a same kind of a material. Will be used as a prefix for cache key.
        // MUST NOT be called inside a ctor or dtor (becase of "*this" and virtual slugImpl())!
        std::string const & slug() const
        {
            if (_materialSlug.empty())
            {
                _materialSlug = fmt::format("{}:{}", typeid(*this).hash_code(),
                                                     slugImpl());
            }
            return _materialSlug;
        }

    protected:
        // Should contain material-specific unique features.
        // It should reflect possible changes in shaders due to
        // differences in _extra and userUniforms.
        virtual std::string slugImpl() const = 0;

    private:
        mutable std::string _materialSlug;
    };
}
