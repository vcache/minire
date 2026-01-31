#pragma once

#include <boost/container_hash/hash.hpp> // for hash_combine
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <functional> // For std::hash
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minire::models { class MeshFeatures; };
namespace minire::opengl { class Program; }
namespace minire::opengl { class Texture; }

// TODO: this abstraction is pretty shitty actually,
//       because it cannot be implemented w/o access
//       to opengl/vulkan low level calls (or their wrappers).
//       So, this abstraction is pretty initial and will be altered.

namespace minire::material
{
    using TextureRefs = std::vector<opengl::Texture const *>;
    using SkinningVector = std::vector<glm::mat4>;

    /**
     * This class is exposed to the user and it contains
     * material-specific parameters, such as texture, colors, and so on.
     * All complex resources (such as textures) should be represented by
     * their content::Id.
     * */
    class Model
    {
    public:
        using Sptr = std::shared_ptr<Model>;
        using Uptr = std::unique_ptr<Model>;

        virtual ~Model() = default;

        virtual std::string const & materialKind() const = 0;
    };

    /**
     * This is a preloaded version of a Model. Instead of content::Id,
     * it stores pointer to GPU-side textures and any additional (unique)
     * information that is required for material rendering.
     * */
    class Instance
    {
    public:
        using Uptr = std::unique_ptr<Instance>;

        virtual ~Instance() = default;
    };

    /**
     * This object represents all non-unique information about material
     * rendering, such as shaders, uniform locations, programs.
     *
     * TODO: maybe rename it to Executer or Runtime or so?
     * */
    class Program
    {
    public:
        using Sptr = std::shared_ptr<Program>;

        virtual ~Program() = default;

        // Should setup uniforms, activate textures, use programs, etc
        // Assuming that the next call will be glDrawArrays or glDrawElements
        virtual void prepareDrawing(Instance const &,
                                    glm::mat4 const & modelTransform,
                                    glm::vec3 const & ambientLight,
                                    glm::vec3 const & emissiveFactor,
                                    TextureRefs const & directionalLightsShadowMaps,
                                    TextureRefs const & pointLightsShadowMaps,
                                    SkinningVector const & skinningVector) const = 0;

        virtual opengl::Program const & glProgram() const = 0;

        // TODO: assert int == GLint
        struct Locations
        {
            int _vertexAttribute  = -1;
            int _uvAttribute      = -1;
            int _normalAttribute  = -1;
            int _tangentAttribute = -1;
            int _jointsAttribute  = -1;
            int _weightsAttribute = -1;

            // NOTE: note the std::hash<Locations>

            bool operator==(Locations const & o) const
            {
                return std::tie(  _vertexAttribute,    _uvAttribute,       _normalAttribute,
                                  _tangentAttribute,   _jointsAttribute,   _weightsAttribute)
                    == std::tie(o._vertexAttribute,  o._uvAttribute,     o._normalAttribute,
                                o._tangentAttribute, o._jointsAttribute, o._weightsAttribute);
            }
        };

        virtual Locations locations() const = 0;
    };

    /**
     * A Factory's function is to transform a user-side Model into
     * a rasterized-side Instance and corresponding Program.
     * */
    class Factory
    {
    public:
        using Uptr = std::unique_ptr<Factory>;

        virtual ~Factory() = default;

        virtual Program::Sptr build(Model const &, models::MeshFeatures const &) const = 0;

        // TODO: why not Sptr?
        virtual Instance::Uptr instantiate(Model const &, models::MeshFeatures const &) const = 0;

        virtual std::string signature(Model const &, models::MeshFeatures const &) const = 0;
    };
}

namespace std
{
    template<>
    struct hash<::minire::material::Program::Locations>
    {
        size_t operator()(::minire::material::Program::Locations const & v) const
        {
            size_t result = 0;
            boost::hash_combine(result, v._vertexAttribute);
            boost::hash_combine(result, v._uvAttribute);
            boost::hash_combine(result, v._normalAttribute);
            boost::hash_combine(result, v._tangentAttribute);
            boost::hash_combine(result, v._jointsAttribute);
            boost::hash_combine(result, v._weightsAttribute);
            return result;
        }
    };
}
