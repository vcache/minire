#include <rasterizer/model.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/std-pair-hash.hpp>

#include <rasterizer/materials.hpp>
#include <utils/gltf-interpreters.hpp>
#include <utils/obj-interpreters.hpp>
#include <utils/overloaded.hpp>

#include <cassert>
#include <variant>

namespace minire::rasterizer
{
    void Model::loadPrimitives(content::Id const & id,
                               models::SceneModel const & sceneModel,
                               content::Manager & contentManager,
                               Materials const & materials,
                               Ubo const & ubo)
    {
        size_t const meshIndex = sceneModel._meshIndex;
        auto const & defaultMaterial = sceneModel._defaultMaterial;

        MINIRE_INFO("Loading a model: {}", sceneModel._source);
        auto lease = contentManager.borrow(sceneModel._source);
        assert(lease);

        _aabb = utils::Aabb();

        return lease->visit(utils::Overloaded
        {
            [this, &id, meshIndex, &defaultMaterial, &materials, &ubo]
            (formats::Obj const & obj)
            {
                MINIRE_INVARIANT(defaultMaterial, "material not specified: {}", id);
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

                _aabb.extend(vertexBuffer._aabb);

                _primitives.emplace_back(Primitive{std::move(vertexBuffer)});
                _materials.emplace_back(Material{std::move(matProgram), std::move(matInstance), {0}});
            },

            [this, &id, meshIndex, &defaultMaterial, &materials, &ubo, &contentManager]
            (formats::GltfModelSptr const & gltf)
            {
                MINIRE_INVARIANT(gltf, "gltf pointer is empty: {}", id);
                MINIRE_INVARIANT(meshIndex != models::SceneModel::kNoIndex,
                                 "gLTF-mesh must have an index: {}", id);

                auto prefetched = utils::prefetchGltfFeatures(gltf, meshIndex, contentManager);

                using MatComboKey = std::pair<models::MeshFeatures, size_t>;
                using MatMap = std::unordered_map<MatComboKey, Material>;
                MatMap materialsMap;
                materialsMap.reserve(prefetched._materialModels.size());
                std::vector<material::Program::Locations> locationsForPrims;
                locationsForPrims.reserve(prefetched._primitives.size());

                for(size_t primIndex = 0; primIndex < prefetched._primitives.size(); ++primIndex)
                {
                    auto const & primitive = prefetched._primitives[primIndex];
                    MatComboKey key(primitive._meshFeatures, primitive._materialModel);
                    auto it = materialsMap.find(key);
                    if (it == materialsMap.cend())
                    {
                        // a new combination found, should build a new material
                        bool const useDefault = primitive._materialModel == utils::GltfMeshFeatures::kNoIndex;
                        MINIRE_INVARIANT(!useDefault || defaultMaterial,"no default material specified: {}", id);

                        assert(useDefault || primitive._materialModel < prefetched._materialModels.size());
                        MINIRE_INVARIANT(useDefault || prefetched._materialModels[primitive._materialModel],
                                         "no builtin material loaded, {}", id);

                        material::Model const & effectiveMaterial =
                            useDefault ? *defaultMaterial
                                       : *prefetched._materialModels[primitive._materialModel];

                        auto matProgram = materials.build(effectiveMaterial, primitive._meshFeatures, ubo);
                        auto matInstance = materials.instantiate(effectiveMaterial, primitive._meshFeatures);

                        MINIRE_INVARIANT(matProgram, "no material program for {}", id);
                        MINIRE_INVARIANT(matInstance, "no material instance for {}", id);

                        Material newMaterial{std::move(matProgram), std::move(matInstance), {}};
                        auto [newIt, inserted] = materialsMap.emplace(key, std::move(newMaterial));
                        MINIRE_INVARIANT(inserted, "failed to insert a new material+feature pair: {}", id);
                        it = newIt;
                    }
                    assert(it != materialsMap.cend());
                    it->second._primitives.emplace_back(primIndex);
                    assert(it->second._matProgram);
                    locationsForPrims.emplace_back(it->second._matProgram->locations());
                }

                for(auto & [_, material] : materialsMap)
                {
                    _materials.emplace_back(std::move(material));
                }

                std::vector<opengl::VertexBuffer> vertexBuffers = utils::createVertexBuffers(
                    *gltf, meshIndex, locationsForPrims);
                assert(vertexBuffers.size() == prefetched._primitives.size());
                _primitives.reserve(vertexBuffers.size());
                for(opengl::VertexBuffer & vertexBuffer : vertexBuffers)
                {
                    _aabb.extend(vertexBuffer._aabb);
                    _primitives.emplace_back(std::move(vertexBuffer));
                }
            },

            [&id](auto const &)
            {
                MINIRE_THROW("unknown mesh format: {}", id);
            }
        });
    }

    Model::Model(content::Id const & id,
                 models::SceneModel const & sceneModel,
                 content::Manager & contentManager,
                 Materials const & materials,
                 Ubo const & ubo)
    {
        loadPrimitives(id, sceneModel, contentManager, materials, ubo);
    }

    void Model::draw(glm::mat4 const & modelTransform,
                     float const colorFactor) const
    {
        for(Material const & material : _materials)
        {
            assert(material._matProgram);
            assert(material._matInstance);
            material._matProgram->prepareDrawing(*(material._matInstance),
                                                   modelTransform,
                                                   colorFactor);
            for(size_t const primIndex : material._primitives)
            {
                assert(primIndex < _primitives.size());
                _primitives[primIndex]._buffer.drawElements();
            }
        }
    }
}
