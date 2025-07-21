#pragma once

#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/models/scene-model.hpp>
#include <minire/utils/aabb.hpp>

#include <opengl/vertex-buffer.hpp>

#include <glm/mat4x4.hpp>

#include <memory>
#include <string>
#include <vector>

namespace minire::content { class Manager; }

namespace minire::rasterizer
{
    class Materials;
    class Ubo;

    class Mesh final
    {
    public:
        using Uptr = std::unique_ptr<Mesh>;

        explicit Mesh(content::Id const & id,
                      models::SceneModel const &,
                      content::Manager &,
                      Materials const &,
                      Ubo const &);

        // assuming that caller will "use" gl's program!
        void draw(glm::mat4 const &,
                  float const colorFactor) const;

        utils::Aabb const & aabb() const { return _aabb; }

    private:
        struct Material
        {
            material::Program::Sptr  _matProgram;
            material::Instance::Uptr _matInstance;
            std::vector<size_t>      _primitives;
        };

        struct Primitive
        {
            opengl::VertexBuffer _buffer;
        };

    private:
        void loadPrimitives(content::Id const & id,
                            models::SceneModel const & sceneModel,
                            content::Manager & contentManager,
                            Materials const & materials,
                            Ubo const & ubo);

    private:
        std::vector<Material>  _materials;
        std::vector<Primitive> _primitives;
        utils::Aabb            _aabb;

        friend class Meshes;
    };
}
