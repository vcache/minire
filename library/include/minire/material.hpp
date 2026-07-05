#pragma once

#include <minire/content/id.hpp>
#include <minire/errors.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/models/sampler.hpp>
#include <minire/utils/demangle.hpp>

#include <fmt/format.h>
#include <glm/ext/matrix_double2x2.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <nlohmann/json_fwd.hpp>

#include <any>
#include <array>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
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

        struct TextureUniform
        {
            content::Id     _textureId;
            models::Sampler _sampler;
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
            TextureUniform,

            // TODO: this is a hack for cube-shadow-map.cpp, should re-desgin this
            //       in a more elegant way
            std::array<glm::mat4, 6>
        >;

        class UserUniform
        {
        public:
            explicit UserUniform(std::string const & name)
                : _name(std::move(name))
                , _value(std::monostate())
                , _updated(false)
            {}

            template<typename T>
            UserUniform(std::string const & name,
                        T && initialValue)
                : _name(std::move(name))
                , _value(std::forward<T>(initialValue))
                , _updated(true)
            {}

            template<typename T>
            void setValue(T && value)
            {
                _value = std::forward<T>(value);
                _updated = true;
            }

            std::string const & name() const { return _name; }
            bool updated() const { return _updated; }
            UniformValue const & value() const { return _value; }

            void markDone() { _updated = false; }

        private:
            std::string const _name;
            UniformValue      _value;
            bool              _updated; // must be set by updateUserUniforms
                                        // if _value has been changed
        };

        class UserUniforms
        {
        public:
            // Names must be pre-allocated to ensure that
            // _uniforms won't be resized.
            template<typename NamesIt>
            explicit UserUniforms(NamesIt namesBegin, NamesIt namesEnd)
                : _uniforms(namesBegin, namesEnd)
            {}

            UserUniform * find(std::string_view);

            std::any & userData() { return _userData; }
            std::any const & userData() const { return _userData; }

            // guarantees non-null result
            template<typename T, typename... Args>
            T * getOrMakeUserData(Args && ... args)
            {
                if (!_userData.has_value())
                {
                    _userData.emplace<T>(std::forward<Args>(args)...);
                }
                T * uniforms = std::any_cast<T>(&_userData);
                MINIRE_INVARIANT(uniforms, "bad any cast: {}", utils::demangle<T>());
                return uniforms;
            }

            auto begin() { return _uniforms.begin(); }
            auto end() { return _uniforms.end(); }

            auto begin() const { return _uniforms.cbegin(); }
            auto end() const { return _uniforms.cend(); }

            size_t size() const { return _uniforms.size(); }

            UserUniform const & operator[](size_t i) const { assert(i < _uniforms.size()); return _uniforms[i]; }
            UserUniform & operator[](size_t i) { assert(i < _uniforms.size()); return _uniforms[i]; }

        private:
            std::vector<UserUniform> _uniforms;
            std::any                 _userData;

            // Since users can store pointers to _uniforms elements,
            // the structure must be protected from pointers drift.
            UserUniforms(UserUniforms const &) = delete;
            UserUniforms(UserUniforms &&) = delete;
            UserUniforms & operator=(UserUniforms const &) = delete;
            UserUniforms & operator=(UserUniforms &&) = delete;
        };

        // A helper for implementations of Material to cache uniform's locations.
        template<typename T, char const * U>
        class UserUniformTracker
        {
        public:
            static constexpr std::string_view kUniformName = U;

            explicit UserUniformTracker(UserUniforms & userUniforms)
                : _userUniforms(userUniforms)
            {}

            template<typename V>
            void set(V && value)
            {
                if (!_destination) _destination = _userUniforms.find(kUniformName);
                MINIRE_INVARIANT(_destination, "no user uniform found: {}", kUniformName);
                _destination->setValue(std::forward<V>(value));
            }

        private:
            UserUniforms & _userUniforms;
            UserUniform *  _destination = nullptr;
        };
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
        // It may be updated fully, partially, or not at all.
        // It is guaranteed that this list, it's contents (including ordering),
        // won't be changed by Material engine, i.e. it is safe to store and access
        // indeces inside this vector (but ONLY for the same "brushId").
        // The list of uniforms is auto-collected by minire_register_user_uniform.
        // Returns true, if at least one uniform has been updated.
        virtual void updateUserUniforms(material::UserUniforms &) const = 0;

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
