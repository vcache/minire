#include <rasterizer/mesh.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/std-pair-hash.hpp>

#include <rasterizer/materials.hpp>
#include <rasterizer/vertex-buffers.hpp>
#include <utils/gltf-interpreters.hpp>
#include <utils/obj-interpreters.hpp>
#include <utils/overloaded.hpp>

#include <cassert>
#include <variant>

namespace minire::rasterizer
{
    void Mesh::loadPrimitives(content::Path const & source,
                              material::Model::Sptr const & defaultMaterial,
                              content::Manager & contentManager,
                              Materials const & materials,
                              Ubo const & ubo,
                              VertexBuffers const & vertexBuffers)
    {
        MINIRE_INVARIANT(!source.empty(), "source path is empty");

        // Special case, loading from VertexBuffers

        if (std::holds_alternative<content::path::Special>(source[0]) &&
            std::get<content::path::Special>(source[0]) == content::path::Special::kVertexBuffers)
        {
            // Sanity check

            MINIRE_INVARIANT(source.size() == 2, "unexpected size of path to vertex-buffers ({}): {}",
                             source.size(), source);
            MINIRE_INVARIANT(std::holds_alternative<content::Id>(source[1]),
                             "unexpected component type of vertex buffer path: {}", source);
            content::Id vertexBufferId = std::get<content::Id>(source[1]);

            // Build material instance and fetch a program
            models::MeshFeatures const & meshFeatures = vertexBuffers.meshFeatures(vertexBufferId);
            auto matProgram = materials.build(*defaultMaterial, meshFeatures, ubo);
            auto matInstance = materials.instantiate(*defaultMaterial, meshFeatures);
            MINIRE_INVARIANT(matProgram, "no material program for {}", source);
            MINIRE_INVARIANT(matInstance, "no material instance for {}", source);

            // Create or make opengl::VertexBuffer for a given Locations
            material::Program::Locations const & locations = matProgram->locations();
            auto openglVertexBuffer = vertexBuffers.build(vertexBufferId, locations);
            assert(openglVertexBuffer);

            // extend aabb
            _aabb.extend(openglVertexBuffer->_aabb);

            // setup _primitives and _materials

            _primitives.emplace_back(std::move(openglVertexBuffer), meshFeatures, locations);
            _materials.emplace_back(Material{std::move(matProgram), std::move(matInstance), {0}});

            return;
        }

        // Regular case, loading from a file

        MINIRE_INVARIANT(std::holds_alternative<content::Id>(source[0]),
                         "source's path doesn't start from Id");

        auto lease = contentManager.borrow(std::get<content::Id>(source[0]));
        assert(lease);

        _aabb = utils::Aabb();

        return lease->visit(utils::Overloaded
        {
            [this, &source, &defaultMaterial, &materials, &ubo]
            (formats::Obj const & obj)
            {
                MINIRE_INVARIANT(defaultMaterial, "no default material found (required for OBJ): {}", source);
                MINIRE_INVARIANT(source.size() == 1, "too many path component for OBJ: {}", source.size());

                models::MeshFeatures meshFeatures = utils::getMeshFeatures(obj);

                auto matProgram = materials.build(*defaultMaterial, meshFeatures, ubo);
                auto matInstance = materials.instantiate(*defaultMaterial, meshFeatures);

                MINIRE_INVARIANT(matProgram, "no material program for {}", source);
                MINIRE_INVARIANT(matInstance, "no material instance for {}", source);

                material::Program::Locations const & locations = matProgram->locations();
                MINIRE_INVARIANT(locations.tangentAttribute() == -1, "OBJ doesn't support tangents");
                MINIRE_INVARIANT(locations.jointsAttribute() == -1, "OBJ doesn't support skining");
                MINIRE_INVARIANT(locations.weightsAttribute() == -1, "OBJ doesn't support skining");
                opengl::VertexBuffer vertexBuffer = utils::createVertexBuffer(
                    obj,
                    locations.vertexAttribute(),
                    locations.uvAttribute(),
                    locations.normalAttribute());

                _aabb.extend(vertexBuffer._aabb);

                _primitives.emplace_back(std::make_shared<opengl::VertexBuffer>(std::move(vertexBuffer)),
                                         meshFeatures, locations);
                _materials.emplace_back(Material{std::move(matProgram), std::move(matInstance), {0}});
            },

            [this, &source, &defaultMaterial, &materials, &ubo, &contentManager]
            (formats::GltfModelSptr const & gltf)
            {
                // Check preconditions
                MINIRE_INVARIANT(gltf, "gltf pointer is empty: {}", source);
                MINIRE_INVARIANT(source.size() == 3, "too few gLTF mesh path components: {}", source.size());
                MINIRE_INVARIANT(std::holds_alternative<content::path::Special>(source[1]) &&
                                 std::get<content::path::Special>(source[1]) == content::path::Special::kMeshes,
                                 "source path doesn't point to a meshes store: {}", source);

                // Fetch mesh index
                size_t const meshIndex = std::visit(utils::Overloaded
                    {
                        [](content::path::Index index) -> size_t { return index; },
                        [&gltf](content::Id const & name) -> size_t
                        {
                            for(size_t i = 0; i < gltf->meshes.size(); ++i)
                            {
                                if (gltf->meshes[i].name == name)
                                    return i;
                            }
                            MINIRE_THROW("no such mesh: \"{}\"", name);
                        },
                        [](auto const &) -> size_t
                        {
                            MINIRE_THROW("unknown kind of mesh id (not a name or index)");
                        }
                    }, source[2]);

                // get features
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
                        MINIRE_INVARIANT(!useDefault || defaultMaterial,"no default material specified: {}", source);

                        assert(useDefault || primitive._materialModel < prefetched._materialModels.size());
                        MINIRE_INVARIANT(useDefault || prefetched._materialModels[primitive._materialModel],
                                         "no builtin material loaded, {}", source);

                        material::Model const & effectiveMaterial =
                            useDefault ? *defaultMaterial
                                       : *prefetched._materialModels[primitive._materialModel];

                        auto matProgram = materials.build(effectiveMaterial, primitive._meshFeatures, ubo);
                        auto matInstance = materials.instantiate(effectiveMaterial, primitive._meshFeatures);

                        MINIRE_INVARIANT(matProgram, "no material program for {}", source);
                        MINIRE_INVARIANT(matInstance, "no material instance for {}", source);

                        Material newMaterial{std::move(matProgram), std::move(matInstance), {}};
                        auto [newIt, inserted] = materialsMap.emplace(key, std::move(newMaterial));
                        MINIRE_INVARIANT(inserted, "failed to insert a new material+feature pair: {}", source);
                        it = newIt;
                    }
                    assert(it != materialsMap.cend());
                    it->second._primitives.emplace_back(primIndex);
                    assert(it->second._matProgram);

                    material::Program::Locations const & locations = it->second._matProgram->locations();
                    locationsForPrims.emplace_back(locations);
                }

                for(auto & [_, material] : materialsMap)
                {
                    _materials.emplace_back(std::move(material));
                }

                std::vector<opengl::VertexBuffer> vertexBuffers = utils::createVertexBuffers(
                    *gltf, meshIndex, locationsForPrims);
                assert(vertexBuffers.size() == prefetched._primitives.size());
                assert(vertexBuffers.size() == locationsForPrims.size());
                _primitives.reserve(vertexBuffers.size());
                for(size_t i = 0; i < vertexBuffers.size(); ++i)
                {
                    opengl::VertexBuffer & vertexBuffer = vertexBuffers[i];
                    _aabb.extend(vertexBuffer._aabb);
                    _primitives.emplace_back(std::make_shared<opengl::VertexBuffer>(std::move(vertexBuffer)),
                                             prefetched._primitives[i]._meshFeatures,
                                             locationsForPrims[i]);
                }
            },

            [&source](auto const &)
            {
                MINIRE_THROW("unknown mesh format: {}", source);
            }
        });
    }

    Mesh::Mesh(content::Path const & source,
               material::Model::Sptr const & defaultMaterial,
               content::Manager & contentManager,
               Materials const & materials,
               Ubo const & ubo,
               VertexBuffers const & vertexBuffers)
    {
        loadPrimitives(source, defaultMaterial, contentManager, materials, ubo, vertexBuffers);
    }

    void Mesh::draw(glm::mat4 const & modelTransform,
                    glm::vec3 const & ambientLight,
                    glm::vec3 const & emissiveFactor,
                    material::TextureRefs const & directionalLightsShadowMaps,
                    material::TextureRefs const & pointLightsShadowMaps,
                    material::SkinningVector const & skinningVector) const
    {
        for(Material const & material : _materials)
        {
            assert(material._matProgram);
            assert(material._matInstance);
            material._matProgram->prepareDrawing(*(material._matInstance),
                                                 modelTransform,
                                                 ambientLight,
                                                 emissiveFactor,
                                                 directionalLightsShadowMaps,
                                                 pointLightsShadowMaps,
                                                 skinningVector);
            for(size_t const primIndex : material._primitives)
            {
                assert(primIndex < _primitives.size());
                assert(_primitives[primIndex]._buffer);
                _primitives[primIndex]._buffer->drawElements();
            }
        }
    }

    void Mesh::drawBare() const
    {
        for(Primitive const & primitive : _primitives)
        {
            assert(primitive._buffer);
            primitive._buffer->drawElements();
        }
    }

    Mesh::PrimitiveTraits Mesh::primitiveTraits(size_t const primitiveIndex) const
    {
        assert(primitiveIndex < _primitives.size());
        Primitive const & primitive = _primitives[primitiveIndex];
        return std::tie(primitive._meshFeatures,
                        primitive._attribLocations);
    }

    void Mesh::drawBare(size_t const primitiveIndex) const
    {
        assert(primitiveIndex < _primitives.size());
        assert(_primitives[primitiveIndex]._buffer);
        _primitives[primitiveIndex]._buffer->drawElements();
    }
}
