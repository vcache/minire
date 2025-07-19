#pragma once

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

namespace minire::opengl { class Program; }
namespace minire::models { class MeshFeatures; };

// TODO: this abstraction is pretty shitty actually,
//       because it cannot be implemented w/o access
//       to opengl/vulkan low level calls (or their wrappers).
//       So, this abstraction is pretty initial and will be altered.

namespace minire::material
{
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
                                    float const colorFactor) const = 0;

        virtual opengl::Program const & glProgram() const = 0;

        // TODO: assert int == GLint
        struct Locations
        {
            int _vertexAttribute;
            int _uvAttribute;
            int _normalAttribute;
            int _tangentAttribute;
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
