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

    class Model final
    {
    public:
        using Uptr = std::unique_ptr<Model>;

        explicit Model(content::Id const & id,
                       models::SceneModel const &,
                       content::Manager &,
                       Materials const &,
                       Ubo const &);

        // assuming that caller will "use" gl's program!
        void draw(glm::mat4 const &,
                  float const colorFactor) const;

        utils::Aabb const & aabb() const { return _aabb; }

    private:
        struct Primitive
        {
            using Uptr = std::unique_ptr<Primitive>;

            material::Program::Sptr  _matProgram;
            material::Instance::Uptr _matInstance;
            opengl::VertexBuffer     _buffers;

            explicit Primitive(material::Program::Sptr && matProgram,
                               material::Instance::Uptr && matInstance,
                               opengl::VertexBuffer && buffers)
                : _matProgram(std::move(matProgram))
                , _matInstance(std::move(matInstance))
                , _buffers(std::move(buffers))
            {}
        };

    private:
        static Primitive::Uptr loadPrimitive(content::Id const & id,
                                             models::SceneModel const & sceneModel,
                                             content::Manager & contentManager,
                                             Materials const & materials,
                                             Ubo const & ubo);

        static std::vector<Primitive::Uptr>
        loadPrimitives(content::Id const & id,
                       models::SceneModel const & sceneModel,
                       content::Manager & contentManager,
                       Materials const & materials,
                       Ubo const & ubo);

    private:
        std::vector<Primitive::Uptr> _primitives;
        utils::Aabb const            _aabb;

        friend class Models;
    };
}
