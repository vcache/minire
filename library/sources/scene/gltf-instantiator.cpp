#include <scene/gltf-instantiator.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/mesh.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/transform.hpp>

#include <scene.hpp>
#include <utils/uuid.hpp>

#include <fmt/ranges.h>
#include <glm/gtc/type_ptr.hpp>

#include <unordered_set>

// TODO: why _visible flag is propagated recursively? Is this behaviour sane?

namespace minire::scene
{
    namespace
    {
        // TODO: move it somewhere into a common area
        events::controller::ScenePath concat(events::controller::ScenePath const & prefix,
                                             std::string suffix)
        {
            auto result = prefix;
            result.emplace_back(suffix);
            return result;
        }

        void instantiateGltfCamera(Scene & scene,
                                   events::controller::SceneNewFromSource const & e,
                                   ::tinygltf::Model const & model,
                                   size_t const cameraIndex)
        {
            MINIRE_INVARIANT(cameraIndex < model.cameras.size(),
                             "bad camera index ({} >= {}): {}",
                             cameraIndex, model.cameras.size(), e._source);
            ::tinygltf::Camera const & camera = model.cameras[cameraIndex];

            if (camera.type == "perspective")
            {
                ::tinygltf::PerspectiveCamera const & pcamera = camera.perspective;
                MINIRE_INVARIANT(pcamera.yfov > 0.0, "bad camera.yfov ({}): {}", pcamera.yfov, e._source);
                MINIRE_INVARIANT(pcamera.znear > 0.0, "bad camera.znear ({}): {}", pcamera.znear, e._source);
                scene.handle(events::controller::SceneNewPerspectiveCamera
                {
                    ._id = camera.name.empty() ? utils::newUuid() : camera.name,
                    ._parent = e._parent,
                    ._data = models::PerspectiveCamera
                    {
                        ._yFov = static_cast<float>(pcamera.yfov),
                        ._zNear = static_cast<float>(pcamera.znear),
                        ._zFar = pcamera.zfar > 0.0 ? std::optional<float>(pcamera.zfar)
                                                    : std::nullopt,
                        ._aspectRatio = pcamera.aspectRatio > 0.0 ? std::optional<float>(pcamera.aspectRatio)
                                                                  : std::nullopt,
                    },
                    ._visible = e._visible,
                });
            }
            else if (camera.type == "orthographic")
            {
                ::tinygltf::OrthographicCamera const & ocamera = camera.orthographic;
                MINIRE_INVARIANT(ocamera.xmag != 0.0, "bad camera.xmag ({}): {}", ocamera.xmag, e._source);
                MINIRE_INVARIANT(ocamera.ymag != 0.0, "bad camera.ymag ({}): {}", ocamera.ymag, e._source);
                MINIRE_INVARIANT(ocamera.zfar > ocamera.znear, "zfar <= znear ({} <= {}): {}",
                                 ocamera.zfar, ocamera.znear, e._source);
                scene.handle(events::controller::SceneNewOrthographicCamera
                {
                    ._id = camera.name.empty() ? utils::newUuid() : camera.name,
                    ._parent = e._parent,
                    ._data = models::OrthographicCamera
                    {
                        ._xMag = static_cast<float>(ocamera.xmag),
                        ._yMag = static_cast<float>(ocamera.ymag),
                        ._zNear = static_cast<float>(ocamera.znear),
                        ._zFar = static_cast<float>(ocamera.zfar),
                    },
                    ._visible = e._visible,
                });
            }
            else
            {
                MINIRE_THROW("unexpected camera type \"{}\": {}",
                             camera.type, e._source);
            }
        }

        void instantiateGltfMesh(Scene & scene,
                                 events::controller::SceneNewFromSource const & e,
                                 ::tinygltf::Model const & model,
                                 size_t const meshIndex)
        {
            MINIRE_INVARIANT(meshIndex < model.meshes.size(),
                             "bad mesh index ({} >= {}): {}",
                             meshIndex, model.meshes.size(), e._source);
            ::tinygltf::Mesh const & mesh = model.meshes[meshIndex];
            scene.handle(events::controller::SceneNewMesh
            {
                ._id = mesh.name.empty() ? utils::newUuid() : mesh.name,
                ._parent = e._parent,
                ._data = models::Mesh
                {
                    ._source = content::mkPath(e._source[0],
                                               content::path::Special::kMeshes,
                                               content::path::Index(meshIndex)),
                    ._defaultMaterial = {},
                },
                ._visible = e._visible,
            });
        }

        // see ref at:
        //     https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
        void instantiateGltfLight(Scene & scene,
                                  events::controller::SceneNewFromSource const & e,
                                  ::tinygltf::Model const & model,
                                  size_t const lightIndex)
        {
            MINIRE_INVARIANT(lightIndex < model.lights.size(),
                             "bad light index ({} >= {}): {}",
                             lightIndex, model.lights.size(), e._source);
            ::tinygltf::Light const & light = model.lights[lightIndex];

            if (light.type == "point")
            {
                models::PointLight pointLight(glm::vec4(1.0f, 1.0f, 1.0f, light.intensity),
                                              light.range != 0.0 ? light.range : 3250); // TODO: default range must be infinite
                if (!light.color.empty())
                {
                    MINIRE_INVARIANT(light.color.size() == 3,
                                     "expected 3 color components, but got {}: {}",
                                     light.color.size(), e._source);
                    pointLight._color[0] = light.color[0];
                    pointLight._color[1] = light.color[1];
                    pointLight._color[2] = light.color[2];
                }

                scene.handle(events::controller::SceneNewPointLight
                {
                    ._id = light.name.empty() ? utils::newUuid() : light.name,
                    ._parent = e._parent,
                    ._data = pointLight,
                    ._visible = e._visible,
                });
            }
            else
            {
                MINIRE_THROW("unsupported light type: \"{}\": {}", light.type, e._source);
            }
        }

        void instantiateGltfNode(Scene & scene,
                                 events::controller::SceneNewFromSource const & e,
                                 ::tinygltf::Model const & model,
                                 size_t const nodeIndex,
                                 std::unordered_set<size_t> stopList = {})
        {
            //  check preconditions

            MINIRE_INVARIANT(!stopList.contains(nodeIndex),
                             "Node's loop detected: {}", stopList);
            stopList.emplace(nodeIndex);

            MINIRE_INVARIANT(nodeIndex < model.nodes.size(),
                             "bad node index ({} >= {}): {}",
                             nodeIndex, model.nodes.size(), e._source);
            ::tinygltf::Node const & node = model.nodes[nodeIndex];

            MINIRE_INVARIANT(node.lods.empty(), "MSFT_lod isn't supported: {}", e._source);
            if (node.emitter >= 0)
            {
                MINIRE_WARNING("audio emitters won't be loaded: {}", e._source);
            }

            // create a node itself

            models::Transform transform;

            if (node.matrix.empty())
            {
                assert(transform._rotation == glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                if (!node.rotation.empty())
                {
                    MINIRE_INVARIANT(node.rotation.size() == 4,
                                     "expected 4 components of rotation, but got {}: {}",
                                     node.rotation.size(), e._source);
                    transform._rotation = glm::quat(node.rotation[3],  // w
                                                    node.rotation[0],  // x
                                                    node.rotation[1],  // y
                                                    node.rotation[2]); // z
                }

                assert(transform._scale == glm::vec3(1.0f));
                if (!node.scale.empty())
                {
                    MINIRE_INVARIANT(node.scale.size() == 3,
                                     "expected 3 components of scale, but got {}: {}",
                                     node.scale.size(), e._source);
                    transform._scale = glm::vec3(node.scale[0],    // x
                                                 node.scale[1],    // y
                                                 node.scale[2]);   // z
                }

                assert(transform._translation == glm::vec3(0.0f));
                if (!node.translation.empty())
                {
                    MINIRE_INVARIANT(node.translation.size() == 3,
                                     "expected 3 components of translation, but got {}: {}",
                                     node.translation.size(), e._source);
                    transform._translation = glm::vec3(node.translation[0],    // x
                                                       node.translation[1],    // y
                                                       node.translation[2]);   // z
                }
            }
            else
            {
                MINIRE_INVARIANT(node.matrix.size() == 16,
                                 "expected 16 components of transform matrix, but got {}: {}",
                                 node.matrix.size(), e._source);
                // a column-major order is both in glTF and GLM
                transform.loadFromMatrix(glm::make_mat4x4(node.matrix.data()));
            }

            events::controller::SceneNewNode newNode
            {
                ._id = node.name.empty() ? utils::newUuid() : node.name,
                ._parent = e._parent,
                ._origin = transform,
                ._visible = e._visible,
            };
            scene.handle(newNode);

            // fill it with leafs

            events::controller::SceneNewFromSource subSource
            {
                ._parent = concat(newNode._parent, newNode._id),
                ._source = e._source,
                ._visible = e._visible,
            };

            if (node.camera >= 0)
            {
                instantiateGltfCamera(scene, subSource, model,
                                      static_cast<size_t>(node.camera));
            }

            if (node.mesh >= 0)
            {
                instantiateGltfMesh(scene, subSource, model,
                                    static_cast<size_t>(node.mesh));
            }

            if (node.light >= 0)
            {
                instantiateGltfLight(scene, subSource, model,
                                     static_cast<size_t>(node.light));
            }

            // fill it with subnode

            for(int subNodeIndex : node.children)
            {
                MINIRE_INVARIANT(subNodeIndex >= 0, "bad sub-node index ({}): {}",
                                 subNodeIndex, e._source);
                instantiateGltfNode(scene, subSource, model,
                                    static_cast<size_t>(subNodeIndex));
            }
        }

        void instantiateGltfScene(Scene & scene,
                                  events::controller::SceneNewFromSource const & e,
                                  ::tinygltf::Model const & model,
                                  size_t const sceneIndex)
        {
            MINIRE_INVARIANT(sceneIndex < model.scenes.size(),
                             "bad scene index ({} >= {}): {}",
                             sceneIndex, model.scenes.size(), e._source);
            ::tinygltf::Scene const & gltfScene = model.scenes[sceneIndex];

            if (!gltfScene.audioEmitters.empty())
            {
                MINIRE_WARNING("{} audio emitters won't be loaded from {}",
                               gltfScene.audioEmitters.size(), e._source);
            }

            for(int nodeIndex : gltfScene.nodes)
            {
                MINIRE_INVARIANT(nodeIndex >= 0, "bad node index ({}): {}", nodeIndex, e._source);
                instantiateGltfNode(scene, e, model, static_cast<size_t>(nodeIndex));
            }
        }
    }

    void instantiateGltf(Scene & scene,
                         events::controller::SceneNewFromSource const & e,
                         content::Manager & contentManager)
    {
        // Precheck the path and fetch glTF file

        content::Path const & path = e._source;

        MINIRE_INVARIANT(!path.empty(), "no content path provided");
        MINIRE_INVARIANT(std::holds_alternative<content::Id>(path[0]),
                         "path isn't started from Id: {}", path);
        auto lease = contentManager.borrow(std::get<content::Id>(path[0]));
        assert(lease);

        formats::GltfModelSptr gltf = lease->as<formats::GltfModelSptr>();
        assert(gltf);

        // Build scene graph from a requested part of a glTF file

        if (path.size() == 1)
        {
            // only name of file specified => load default scene
            MINIRE_INVARIANT(gltf->defaultScene >= 0, "no default scene ({}): {}",
                             gltf->defaultScene, path);
            size_t const defaultScene = static_cast<size_t>(gltf->defaultScene);
            instantiateGltfScene(scene, e, *gltf, defaultScene);
        }
        else
        {
            // tail components in a path after a filename => should load some part of glTF file
            MINIRE_INVARIANT(path.size() == 3, "unexpected gLTF path format: {}", path);

            // fetch a kind of a file part to instantiated
            MINIRE_INVARIANT(std::holds_alternative<content::path::Special>(path[1]),
                             "unexpected second component type: {}", path);
            content::path::Special const collection = std::get<content::path::Special>(path[1]);

            // fetch a index of the instantiating element
            MINIRE_INVARIANT(std::holds_alternative<content::path::Index>(path[2]),
                             "unexpected third component type: {}", path);
            size_t const index = std::get<content::path::Index>(path[2]);

            switch(collection)
            {
                case content::path::Special::kCameras:
                    instantiateGltfCamera(scene, e, *gltf, index);
                    break;

                case content::path::Special::kLights:
                    instantiateGltfLight(scene, e, *gltf, index);
                    break;

                case content::path::Special::kMeshes:
                    instantiateGltfMesh(scene, e, *gltf, index);
                    break;

                case content::path::Special::kNodes:
                    instantiateGltfNode(scene, e, *gltf, index);
                    break;

                case content::path::Special::kScenes:
                    instantiateGltfScene(scene, e, *gltf, index);
                    break;
            }
        }

    }
}
