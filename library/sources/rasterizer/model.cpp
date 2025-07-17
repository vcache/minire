#include <rasterizer/model.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <rasterizer/materials.hpp>
#include <utils/gltf-interpreters.hpp>
#include <utils/obj-interpreters.hpp>
#include <utils/overloaded.hpp>

#include <cassert>
#include <variant>

namespace minire::rasterizer
{
    Model::Primitive::Uptr Model::loadPrimitive(content::Id const & id,
                                                models::SceneModel const & sceneModel,
                                                content::Manager & contentManager,
                                                Materials const & materials,
                                                Ubo const & ubo)
    {
        size_t const meshIndex = sceneModel._meshIndex;
        auto const & defaultMaterial = sceneModel._defaultMaterial;
        MINIRE_INVARIANT(defaultMaterial, "material not specified: {}", id); // TODO: shouldn't be mandatory

        MINIRE_INFO("Loading a model: {}", sceneModel._source);
        auto lease = contentManager.borrow(sceneModel._source);
        assert(lease);

        return lease->visit(utils::Overloaded
        {
            [&id, meshIndex, &defaultMaterial, &materials, &ubo]
            (formats::Obj const & obj)
            {
                MINIRE_INVARIANT(meshIndex == models::SceneModel::kNoIndex,
                                 "OBJ-mesh cannot have an index: {}", id);

                models::MeshFeatures meshFeatures = utils::getMeshFeatures(obj);
                auto matProgram = materials.build(*defaultMaterial, meshFeatures, ubo);
                auto matInstance = materials.instantiate(*defaultMaterial, meshFeatures);

                MINIRE_INVARIANT(matProgram, "no material program for {}", id);
                MINIRE_INVARIANT(matInstance, "no material instance for {}", id);

                material::Program::Locations const & locations = matProgram->locations();
                opengl::VertexBuffer vertexBuffer = utils::createVertexBuffer(
                    obj,
                    locations._vertexAttribute,
                    locations._uvAttribute,
                    locations._normalAttribute);

                return std::make_unique<Primitive>(std::move(matProgram),
                                                   std::move(matInstance),
                                                   std::move(vertexBuffer));
            },
            [&id, meshIndex, &defaultMaterial, &materials, &ubo]
            (formats::GltfModelSptr const & gltf)
            {
                MINIRE_INVARIANT(gltf, "gltf pointer is empty: {}", id);
                MINIRE_INVARIANT(meshIndex != models::SceneModel::kNoIndex,
                                 "gLTF-mesh must have an index: {}", id);

                models::MeshFeatures meshFeatures = utils::getMeshFeatures(*gltf, meshIndex);
                auto matProgram = materials.build(*defaultMaterial, meshFeatures, ubo);
                auto matInstance = materials.instantiate(*defaultMaterial, meshFeatures);

                MINIRE_INVARIANT(matProgram, "no material program for {}", id);
                MINIRE_INVARIANT(matInstance, "no material instance for {}", id);

                material::Program::Locations const & locations = matProgram->locations();
                opengl::VertexBuffer vertexBuffer = utils::createVertexBuffer(
                    *gltf, meshIndex,
                    locations._vertexAttribute,
                    locations._uvAttribute,
                    locations._normalAttribute,
                    locations._tangentAttribute);

                return std::make_unique<Primitive>(std::move(matProgram),
                                                   std::move(matInstance),
                                                   std::move(vertexBuffer));
            },
            [&id](auto const &) -> Primitive::Uptr
            {
                MINIRE_THROW("unknown mesh format: {}", id);
            }
        });
    }

    std::vector<Model::Primitive::Uptr>
    Model::loadPrimitives(content::Id const & id,
                          models::SceneModel const & sceneModel,
                          content::Manager & contentManager,
                          Materials const & materials,
                          Ubo const & ubo)
    {
        std::vector<Primitive::Uptr> result;
        result.emplace_back(loadPrimitive(id, sceneModel, contentManager, materials, ubo));
        return result;
    }

    Model::Model(content::Id const & id,
                 models::SceneModel const & sceneModel,
                 content::Manager & contentManager,
                 Materials const & materials,
                 Ubo const & ubo)
        : _primitives(loadPrimitives(id, sceneModel, contentManager, materials, ubo))
        , _aabb(_primitives[0]->_buffers.aabb())
    {}

    void Model::draw(glm::mat4 const & modelTransform,
                     float const colorFactor) const
    {
        for(Primitive::Uptr const & primitive : _primitives)
        {
            assert(primitive);
            assert(primitive->_matProgram);
            assert(primitive->_matInstance);

            primitive->_matProgram->prepareDrawing(*(primitive->_matInstance),
                                                   modelTransform,
                                                   colorFactor);
            primitive->_buffers.drawElements();
        }
    }
}
