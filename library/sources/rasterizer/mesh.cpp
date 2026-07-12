#include <rasterizer/mesh.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/overloaded.hpp>
#include <minire/utils/std-pair-hash.hpp>

#include <rasterizer/materials.hpp>
#include <rasterizer/vertex-buffers.hpp>
#include <utils/gltf-interpreters.hpp>
#include <utils/obj-interpreters.hpp>

#include <cassert>
#include <limits>
#include <variant>

namespace minire::rasterizer
{
    void Mesh::loadPrimitives(content::Path const & source,
                              Material::Sptr const & defaultMaterial,
                              content::Manager & contentManager,
                              Materials const & materials,
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

            MINIRE_INVARIANT(defaultMaterial, "no default material found (required for OBJ): {}", source);
            auto brush = materials.getBrush(meshFeatures, defaultMaterial);
            assert(brush);

            // Create or make opengl::VertexBuffer for a given Locations
            auto openglVertexBuffer = vertexBuffers.build(vertexBufferId, materials.locations());
            assert(openglVertexBuffer);

            // extend aabb
            _aabb.extend(openglVertexBuffer->_aabb);

            // setup _primitives and _materials
            _primitives.emplace_back(std::move(openglVertexBuffer), meshFeatures, defaultMaterial, brush);

            // quit the function, since it is a special case
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
            [this, &source, &defaultMaterial, &materials]
            (formats::Obj const & obj)
            {
                MINIRE_INVARIANT(defaultMaterial, "no default material found (required for OBJ): {}", source);
                MINIRE_INVARIANT(source.size() == 1, "too many path component for OBJ: {}", source.size());

                models::MeshFeatures meshFeatures = utils::getMeshFeatures(obj);
                auto brush = materials.getBrush(meshFeatures, defaultMaterial);
                assert(brush);

                material::Locations const & locations = materials.locations();
                opengl::VertexBuffer vertexBuffer = utils::createVertexBuffer(
                    obj,
                    locations.vertexAttribute(),
                    locations.uvAttribute(),
                    locations.normalAttribute());

                _aabb.extend(vertexBuffer._aabb);

                _primitives.emplace_back(std::make_shared<opengl::VertexBuffer>(std::move(vertexBuffer)),
                                         meshFeatures, defaultMaterial, brush);
            },

            [this, &source, &defaultMaterial, &materials, &contentManager]
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

                // brushes cache
                using MatBrushPair = std::pair<Material::Sptr, Materials::Brush::Sptr>;
                using MatComboKey = std::pair<models::MeshFeatures, size_t>;
                using BrushMap = std::unordered_map<MatComboKey, MatBrushPair>;
                BrushMap materialsMap;
                materialsMap.reserve(prefetched._materialModels.size());

                // brushes vector
                std::vector<MatBrushPair> matBrushesForPrims;
                matBrushesForPrims.reserve(prefetched._primitives.size());

                // iterate primitive to build helper structures
                for(size_t primIndex = 0; primIndex < prefetched._primitives.size(); ++primIndex)
                {
                    auto const & primitive = prefetched._primitives[primIndex];
                    MatComboKey const key(primitive._meshFeatures, primitive._materialModel);
                    MatBrushPair & matBrushPair = materialsMap[key];
                    if (!matBrushPair.first)
                    {
                        assert(!matBrushPair.second);

                        // a new combination found, should build a new material
                        bool const useDefault = primitive._materialModel == utils::GltfMeshFeatures::kNoIndex;
                        MINIRE_INVARIANT(!useDefault || defaultMaterial,"no default material specified: {}", source);

                        assert(useDefault || primitive._materialModel < prefetched._materialModels.size());
                        MINIRE_INVARIANT(useDefault || prefetched._materialModels[primitive._materialModel],
                                         "no builtin material loaded, {}", source);

                        Material::Sptr const & effectiveMaterial =
                            useDefault ? defaultMaterial
                                       : prefetched._materialModels[primitive._materialModel];

                        assert(effectiveMaterial);
                        matBrushPair.first = effectiveMaterial;
                        matBrushPair.second = materials.getBrush(primitive._meshFeatures, effectiveMaterial);
                    }

                    assert(matBrushPair.first);
                    assert(matBrushPair.second);
                    matBrushesForPrims.emplace_back(matBrushPair.first, matBrushPair.second);
                }

                // fetch vertex buffers
                std::vector<opengl::VertexBuffer> vertexBuffers = utils::createVertexBuffers(
                    *gltf, meshIndex, materials.locations());

                // build Mesh::Primitive objects
                assert(vertexBuffers.size() == prefetched._primitives.size());
                assert(vertexBuffers.size() == matBrushesForPrims.size());
                _primitives.reserve(vertexBuffers.size());
                for(size_t i = 0; i < vertexBuffers.size(); ++i)
                {
                    opengl::VertexBuffer & vertexBuffer = vertexBuffers[i];
                    _aabb.extend(vertexBuffer._aabb);
                    _primitives.emplace_back(std::make_shared<opengl::VertexBuffer>(std::move(vertexBuffer)),
                                             prefetched._primitives[i]._meshFeatures, matBrushesForPrims[i].first,
                                             matBrushesForPrims[i].second);
                }
            },

            [&source](auto const &)
            {
                MINIRE_THROW("unknown mesh format: {}", source);
            }
        });
    }

    Mesh::Mesh(content::Path const & source,
               Material::Sptr const & defaultMaterial,
               content::Manager & contentManager,
               Materials const & materials,
               VertexBuffers const & vertexBuffers)
    {
        loadPrimitives(source, defaultMaterial, contentManager, materials, vertexBuffers);
    }

    size_t Mesh::issueConsumerKey()
    {
        static size_t gNextConsumerKey = 0;
        assert(gNextConsumerKey != std::numeric_limits<size_t>::max());
        MINIRE_DEBUG("new mesh consumer key issued: {}", gNextConsumerKey);
        return gNextConsumerKey++;
    }
}
