#include <minire/utils/aabb-tools.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/formats/gltf.hpp>
#include <minire/formats/obj.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/overloaded.hpp>

#include <utils/gltf-element-fetch.hpp>
#include <utils/gltf-node-transform.hpp>

#include <fmt/ranges.h>

#include <cassert>
#include <memory>
#include <type_traits>
#include <unordered_set>
#include <variant>
#include <vector>

namespace minire::utils
{
    namespace
    {
        Aabb buildAabb(formats::Obj const & obj)
        {
            Aabb aabb;
            for(formats::Obj::Vertex const & v : obj._vertices)
            {
                aabb.extend(v);
            }
            return aabb;
        }

        Aabb buildMeshAabb(::tinygltf::Model const & model,
                           content::path::Component const & meshId)
        {
            Aabb result;

            size_t const meshIndex = utils::getElementIndex(meshId, model.meshes, "mesh");
            ::tinygltf::Mesh const & mesh = model.meshes[meshIndex];

            for(::tinygltf::Primitive const & primitive : mesh.primitives)
            {
                if (auto attributesIt = primitive.attributes.find("POSITION");
                    attributesIt != primitive.attributes.cend())
                {
                    MINIRE_INVARIANT(attributesIt->second >= 0, "bad POSITION accessor index: {} at {}",
                                     attributesIt->second, mesh.name);
                    size_t const accessorIndex = static_cast<size_t>(attributesIt->second);
                    MINIRE_INVARIANT(accessorIndex < model.accessors.size(),
                                     "bad POSITION accessor: {} >= {} at {}",
                                     attributesIt->second, model.accessors.size(), mesh.name);
                    ::tinygltf::Accessor const & accessor = model.accessors[accessorIndex];

                    // TODO: this code is duplicated w/ calcAabb() in gltf-interpreters.cpp
                    MINIRE_INVARIANT(accessor.type == TINYGLTF_TYPE_VEC3,
                                     "position isn't Vec3: {}, {}/{}",
                                     accessor.type, mesh.name, accessor.name);

                    std::vector<double> const & min = accessor.minValues;
                    std::vector<double> const & max = accessor.maxValues;

                    MINIRE_INVARIANT(min.size() == 3, "minValues are not 3: {}, {}/{}",
                                     min.size(), mesh.name, accessor.name);

                    MINIRE_INVARIANT(max.size() == 3, "maxValues are not 3: {}, {}/{}",
                                     max.size(), mesh.name, accessor.name);

                    result.extend(Aabb(glm::vec3{min[0], min[1], min[2]},
                                       glm::vec3{max[0], max[1], max[2]}));
                }
            }

            return result;
        }

        Aabb buildNodeAabb(::tinygltf::Model const & model,
                           content::path::Component const & nodeId,
                           std::unordered_set<size_t> & visited,
                           glm::mat4 const & parentMatrix = glm::identity<glm::mat4>())
        {
            Aabb result;

            size_t const nodeIndex = utils::getElementIndex(nodeId, model.nodes, "node");

            MINIRE_INVARIANT(!visited.contains(nodeIndex), "node loop detected in glTF file: {} ({})",
                             nodeIndex, visited);
            visited.insert(nodeIndex);

            ::tinygltf::Node const & node = model.nodes[nodeIndex];

            models::Transform localTransform = utils::getNodeTransform(node);
            glm::mat4 matrix = parentMatrix * localTransform.matrix();

            if (node.mesh >= 0)
            {
                Aabb meshAabb = buildMeshAabb(model, static_cast<size_t>(node.mesh));
                meshAabb.transform(matrix);
                result.extend(meshAabb);
            }

            for(int subNodeIndex : node.children)
            {
                MINIRE_INVARIANT(subNodeIndex >= 0, "bad sub-node index ({}): {}",
                                 subNodeIndex, node.name);
                Aabb subNodeAabb = buildNodeAabb(model, static_cast<size_t>(subNodeIndex),
                                                 visited, matrix);
                result.extend(subNodeAabb);
            }

            return result;
        }

        Aabb buildSceneAabb(::tinygltf::Model const & model,
                            content::path::Component const & sceneId)
        {
            Aabb result;

            size_t const sceneIndex = utils::getElementIndex(sceneId, model.scenes, "scene");
            ::tinygltf::Scene const & gltfScene = model.scenes[sceneIndex];

            std::unordered_set<size_t> visited;
            for(int nodeIndex : gltfScene.nodes)
            {
                MINIRE_INVARIANT(nodeIndex >= 0, "bad node index: {}", nodeIndex);
                Aabb nodeAabb = buildNodeAabb(model, static_cast<size_t>(nodeIndex), visited);
                result.extend(nodeAabb);
            }

            return result;
        }

        Aabb buildAabb(::tinygltf::Model const & model,
                       content::Path const & path)
        {
            assert(!path.empty());

            // only name of file specified => load default scene
            if (path.size() == 1)
            {
                MINIRE_INVARIANT(model.defaultScene >= 0, "no default scene ({}): {}",
                                 model.defaultScene, path);
                size_t const defaultScene = static_cast<size_t>(model.defaultScene);
                return buildSceneAabb(model, defaultScene);
            }

            // tail components in a path after a filename => should load some part of glTF file
            MINIRE_INVARIANT(path.size() == 3, "unexpected glTF path format: {}", path);

            // fetch a kind of a file part to instantiated
            MINIRE_INVARIANT(std::holds_alternative<content::path::Special>(path[1]),
                             "unexpected second component type: {}", path);
            content::path::Special const collection = std::get<content::path::Special>(path[1]);

            std::unordered_set<size_t> visited;
            switch(collection)
            {
                case content::path::Special::kCameras:
                    MINIRE_THROW("cannot calc AABB for a camera: {}", path);

                case content::path::Special::kLights:
                    MINIRE_THROW("cannot calc AABB for a light: {}", path);

                case content::path::Special::kMeshes:
                    return buildMeshAabb(model, path[2]);

                case content::path::Special::kNodes:
                    return buildNodeAabb(model, path[2], visited);

                case content::path::Special::kScenes:
                    return buildSceneAabb(model, path[2]);

                case content::path::Special::kVertexBuffers:
                    MINIRE_THROW("vertex-buffer cannot be a part of glTF collection: {}", path);
            }

            MINIRE_THROW("cannot calc AABB for an unknown gLTF item: {}", path);
        }
    }

    Aabb buildAabb(content::Manager & contentManager,
                   content::Path const & path)
    {
        MINIRE_INVARIANT(!path.empty(), "no content path provided");
        MINIRE_INVARIANT(std::holds_alternative<content::Id>(path[0]),
                         "path isn't started from an Id: {}", path);

        auto lease = contentManager.borrow(std::get<content::Id>(path[0]));
        assert(lease);

        return lease->visit(utils::Overloaded
        {
            [](formats::Obj const & obj) -> Aabb { return buildAabb(obj); },
            [&path](formats::GltfModelSptr const & model) -> Aabb
            {
                MINIRE_INVARIANT(model, "empty gLTF model inside an Asset");
                return buildAabb(*model, path);
            },
            [&path](auto const & asset) -> Aabb
            {
                using T = std::decay_t<decltype(asset)>;
                MINIRE_THROW("cannot buildAabb for {} (the type is {})",
                             path, demangle<T>());
            }
        });
    }
}
