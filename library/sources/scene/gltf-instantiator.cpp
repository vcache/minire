#include <scene/gltf-instantiator.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/models/animations.hpp>
#include <minire/models/camera.hpp>
#include <minire/models/interpolation.hpp>
#include <minire/models/mesh.hpp>
#include <minire/models/point-light.hpp>
#include <minire/models/transform.hpp>
#include <minire/utils/demangle.hpp>

#include <scene.hpp>
#include <utils/gltf-buffer-reader.hpp>
#include <utils/gltf-element-fetch.hpp>
#include <utils/gltf-node-transform.hpp>
#include <utils/overloaded.hpp>
#include <utils/uuid.hpp>

#include <fmt/ranges.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

// TODO: why _visible flag is propagated recursively? Is this behaviour sane?

namespace minire::scene
{
    namespace
    {
        struct Context
        {
            using BufferData = std::variant<
                std::shared_ptr<std::vector<float> const>,
                std::shared_ptr<std::vector<glm::vec3> const>,
                std::shared_ptr<std::vector<glm::mat4> const>,
                std::shared_ptr<std::vector<glm::quat> const>
            >;

            using NodeIndexToScenePath = std::unordered_map<size_t /* node index */,
                                                            models::ScenePath>;
            using BufferDataCache = std::unordered_map<size_t /* accessor index */,
                                                       BufferData>;
            using PendedSkins = std::unordered_map<models::ScenePath, /* mesh */
                                                   size_t /* skin index in the glTF Model */>;

            NodeIndexToScenePath _nodeIndexToScenePath;
            BufferDataCache      _bufferDataCache;
            PendedSkins          _pendedSkins;
        };

        models::Interpolation readInterpolation(std::string const & in)
        {
            if (in == "STEP") return models::Interpolation::kStep;
            if (in == "LINEAR") return models::Interpolation::kLinear;
            if (in == "CUBICSPLINE") return models::Interpolation::kCubic;

            MINIRE_THROW("unexpected interpolation type: \"{}\"", in);
        }

        template<typename T>
        std::shared_ptr<std::vector<T> const> readAccessor(int accessorIndex,
                                                           ::tinygltf::Model const & model,
                                                           Context & context)
        {
            auto it = context._bufferDataCache.find(accessorIndex);
            if (it == context._bufferDataCache.cend())
            {
                std::shared_ptr<std::vector<T> const> data = utils::readAccessor<T>(
                    accessorIndex, model);
                auto [newIt, inserted] = context._bufferDataCache.emplace(accessorIndex, data);
                MINIRE_INVARIANT(inserted, "broken _bufferDataCache");
                it = newIt;

            }
            return std::get<std::shared_ptr<std::vector<T> const>>(it->second);
        }

        void instantiateGltfCamera(Scene & scene,
                                   events::controller::SceneNewFromSource const & e,
                                   ::tinygltf::Model const & model,
                                   content::path::Component const & cameraId)
        {
            size_t const cameraIndex = utils::getElementIndex(cameraId, model.cameras, "camera");
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

        models::ScenePath instantiateGltfMesh(Scene & scene,
                                              events::controller::SceneNewFromSource const & e,
                                              ::tinygltf::Model const & model,
                                              content::path::Component const & meshId)
        {
            size_t const meshIndex = utils::getElementIndex(meshId, model.meshes, "mesh");
            ::tinygltf::Mesh const & mesh = model.meshes[meshIndex];
            events::controller::SceneNewMesh const event
            {
                ._id = mesh.name.empty() ? utils::newUuid() : mesh.name,
                ._parent = e._parent,
                ._data = models::Mesh
                {
                    ._source = content::mkPath(e._source[0],
                                               content::path::Special::kMeshes,
                                               meshId),
                    ._defaultMaterial = {},
                },
                ._visible = e._visible,
            };
            scene.handle(event);
            return models::concat(event._parent, event._id);
        }

        // see ref at:
        //     https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
        void instantiateGltfLight(Scene & scene,
                                  events::controller::SceneNewFromSource const & e,
                                  ::tinygltf::Model const & model,
                                  content::path::Component const & lightId)
        {
            size_t const lightIndex = utils::getElementIndex(lightId, model.lights, "camera");
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
                                 content::path::Component const & nodeId,
                                 bool loadAnimations,
                                 Context & context)
        {
            //  check preconditions
            size_t const nodeIndex = utils::getElementIndex(nodeId, model.nodes, "node");
            ::tinygltf::Node const & node = model.nodes[nodeIndex];

            MINIRE_INVARIANT(node.lods.empty(), "MSFT_lod isn't supported: {}", e._source);
            if (node.emitter >= 0)
            {
                MINIRE_WARNING("audio emitters won't be loaded: {}", e._source);
            }

            // create a node itself

            models::Transform transform = utils::getNodeTransform(node);

            events::controller::SceneNewNode newNode
            {
                ._id = node.name.empty() ? utils::newUuid() : node.name,
                ._parent = e._parent,
                ._origin = transform,
                ._visible = e._visible,
            };
            scene.handle(newNode);
            {
                auto [_, inserted] = context._nodeIndexToScenePath.emplace(
                    nodeIndex, models::concat(newNode._parent, newNode._id));
                MINIRE_INVARIANT(inserted, "broken nodes network ({})", nodeIndex);
            }

            // fill it with leafs

            events::controller::SceneNewFromSource subSource
            {
                ._parent = models::concat(newNode._parent, newNode._id),
                ._source = e._source,
                ._visible = e._visible,
            };

            if (node.camera >= 0)
            {
                instantiateGltfCamera(scene, subSource, model,
                                      static_cast<size_t>(node.camera));
            }

            models::ScenePath meshPath;
            if (node.mesh >= 0)
            {
                meshPath = instantiateGltfMesh(scene, subSource, model,
                                               static_cast<size_t>(node.mesh));
            }

            if (node.light >= 0)
            {
                instantiateGltfLight(scene, subSource, model,
                                     static_cast<size_t>(node.light));
            }

            if (node.skin >= 0)
            {
                MINIRE_INVARIANT(node.mesh >= 0, "cannot have a Skin without a Mesh");
                MINIRE_INVARIANT(!meshPath.empty(), "expected to have a valid mesh path");
                auto [_, inserted] = context._pendedSkins.emplace(meshPath, static_cast<size_t>(node.skin));
                MINIRE_INVARIANT(inserted, "failed to enqueue skin #{} for {}", node.skin, meshPath);
            }

            // fill it with subnode

            for(int subNodeIndex : node.children)
            {
                MINIRE_INVARIANT(subNodeIndex >= 0, "bad sub-node index ({}): {}",
                                 subNodeIndex, e._source);
                instantiateGltfNode(scene, subSource, model,
                                    static_cast<size_t>(subNodeIndex),
                                    false, context);
            }

            // maybe setup animations

            if (loadAnimations)
            {
                // collect animations to be loaded
                std::vector<size_t> animationIndeces;
                animationIndeces.reserve(model.animations.size());
                for(size_t animationIndex = 0;
                    animationIndex < model.animations.size();
                    ++animationIndex)
                {
                    ::tinygltf::Animation const & animation = model.animations[animationIndex];
                    for (::tinygltf::AnimationChannel const & channel : animation.channels)
                    {
                        if (channel.target_node >= 0 &&
                            context._nodeIndexToScenePath.contains(channel.target_node))
                        {
                            animationIndeces.emplace_back(animationIndex);
                            break;
                        }
                    }
                }

                // load the animations
                models::AnimationSet animationSet;
                animationSet.reserve(animationIndeces.size());
                for(size_t animationIndex : animationIndeces)
                {
                    assert(animationIndex < model.animations.size());
                    ::tinygltf::Animation const & animation = model.animations[animationIndex];

                    models::AnimationTracks animationTracks;
                    animationTracks.reserve(animation.channels.size());
                    for(::tinygltf::AnimationChannel const & channel : animation.channels)
                    {
                        // fetch target node
                        MINIRE_INVARIANT(channel.target_node >= 0, "bad target_node: {}, {}",
                                         channel.target_node, animation.name);

                        // fetch target node's ScenePath
                        auto scenePathIt = context._nodeIndexToScenePath.find(channel.target_node);
                        MINIRE_INVARIANT(scenePathIt != context._nodeIndexToScenePath.cend(),
                                         "no ScenePath mapping for {}: {}",
                                         channel.target_node, animation.name);
                        models::ScenePath scenePath = models::cutPrefix(scenePathIt->second, subSource._parent);

                        // fetch AnimationSampler
                        MINIRE_INVARIANT(channel.sampler >= 0, "bad animation sampler: {}, {}",
                                         channel.sampler, animation.name);
                        size_t const samplerIndex = static_cast<size_t>(channel.sampler);
                        MINIRE_INVARIANT(samplerIndex < animation.samplers.size(),
                                         "bad animation sampler: {} >= {}, {}",
                                         samplerIndex, animation.samplers.size(), animation.name);
                        ::tinygltf::AnimationSampler const & animationSampler = animation.samplers[samplerIndex];

                        // build animation tracks
                        models::KeyframeAnimation & keyframeAnimation = animationTracks[scenePath];
                        using TimelineType = models::KeyframeAnimation::Timeline::element_type::value_type;
                        keyframeAnimation._timeline = readAccessor<TimelineType>(animationSampler.input, model, context);

                        if (channel.target_path == "translation")
                        {
                            MINIRE_INVARIANT(!keyframeAnimation._translation,
                                             "multiple animation channels for a target node isn't supported: {}",
                                             animation.name);
                            using T = models::KeyframeAnimation::TranslationTrack::value_type::ValueType;
                            keyframeAnimation._translation.emplace(readAccessor<T>(animationSampler.output, model, context),
                                                                   readInterpolation(animationSampler.interpolation));
                        }
                        else if (channel.target_path == "rotation")
                        {
                            MINIRE_INVARIANT(!keyframeAnimation._rotation,
                                             "multiple animation channels for a target node isn't supported: {}",
                                             animation.name);
                            using T = models::KeyframeAnimation::RotationTrack::value_type::ValueType;
                            keyframeAnimation._rotation.emplace(readAccessor<T>(animationSampler.output, model, context),
                                                                readInterpolation(animationSampler.interpolation));
                        }
                        else if (channel.target_path == "scale")
                        {
                            MINIRE_INVARIANT(!keyframeAnimation._scale,
                                             "multiple animation channels for a target node isn't supported: {}",
                                             animation.name);
                            using T = models::KeyframeAnimation::ScaleTrack::value_type::ValueType;
                            keyframeAnimation._scale.emplace(readAccessor<T>(animationSampler.output, model, context),
                                                             readInterpolation(animationSampler.interpolation));
                        }
                        // TODO: support "pointer"
                        else
                        {
                            MINIRE_THROW("unknown animation channel target_path: \"{}\", {}",
                                         channel.target_path, animation.name);
                        }
                    }

                    animationSet.emplace(animation.name.empty() ? utils::newUuid() : animation.name,
                                         std::move(animationTracks));
                }

                // upload animations
                scene.handle(events::controller::SceneNewAnimationSet{
                    subSource._parent, std::move(animationSet)
                });
            }
        }

        void instantiateGltfScene(Scene & scene,
                                  events::controller::SceneNewFromSource const & e,
                                  ::tinygltf::Model const & model,
                                  content::path::Component const & sceneId,
                                  Context & context)
        {
            size_t const sceneIndex = utils::getElementIndex(sceneId, model.scenes, "scene");
            ::tinygltf::Scene const & gltfScene = model.scenes[sceneIndex];

            if (!gltfScene.audioEmitters.empty())
            {
                MINIRE_WARNING("{} audio emitters won't be loaded from {}",
                               gltfScene.audioEmitters.size(), e._source);
            }

            for(int nodeIndex : gltfScene.nodes)
            {
                MINIRE_INVARIANT(nodeIndex >= 0, "bad node index ({}): {}", nodeIndex, e._source);
                instantiateGltfNode(scene, e, model, static_cast<size_t>(nodeIndex), true, context);
            }
        }

        void attachPendedSkins(Scene & scene, ::tinygltf::Model const & model,
                               Context & context)
        {
            for (auto const & [meshPath, skinIndex] : context._pendedSkins)
            {
                // fetch the skin data
                MINIRE_INVARIANT(skinIndex < model.skins.size(), "bad skin index ({} >= {})",
                                 skinIndex, model.skins.size());
                ::tinygltf::Skin const & skin = model.skins[skinIndex];

                // find a 'skeleton' node (if any)
                std::optional<models::ScenePath> origin;
                if (skin.skeleton >= 0)
                {
                    auto it = context._nodeIndexToScenePath.find(static_cast<size_t>(skin.skeleton));
                    MINIRE_INVARIANT(it != context._nodeIndexToScenePath.cend(),
                                     "skeleton node #{} of Skin \"{}\" isn't instantiated",
                                     skin.skeleton, skin.name);
                    origin = it->second;
                }

                // fetch a list of Inverse Bind Matrices
                std::shared_ptr<std::vector<glm::mat4> const> inverseBindMatrices;
                if (skin.inverseBindMatrices >= 0)
                {
                    inverseBindMatrices = readAccessor<glm::mat4>(skin.inverseBindMatrices, model, context);
                    MINIRE_INVARIANT(inverseBindMatrices, "inverseBindMatrices is broken");
                    // NOTE: The number of elements of the accessor referenced
                    //       by inverseBindMatrices MUST greater than or equal
                    //       to the number of joints elements.
                    MINIRE_INVARIANT(inverseBindMatrices->size() >= skin.joints.size(),
                                     "inverseBindMatrices too short ({} < {})",
                                     inverseBindMatrices->size(), skin.joints.size());
                }

                // load skin's bones
                models::MeshSkin::Bones bones;
                bones.reserve(skin.joints.size());
                for(size_t i = 0; i < skin.joints.size(); ++i)
                {
                    MINIRE_INVARIANT(skin.joints[i] >= 0, "bad joint index: #{}: {}",
                                     i, skin.joints[i]);
                    size_t const jointIndex = static_cast<size_t>(skin.joints[i]);
                    auto it = context._nodeIndexToScenePath.find(jointIndex);
                    MINIRE_INVARIANT(it != context._nodeIndexToScenePath.cend(),
                                     "joint node (id = {}) of Skin \"{}\" isn't instantiated",
                                     jointIndex, skin.name);

                    // NOTE: it is essential to keep bones order as in skins.joints!
                    bones.emplace_back(models::MeshSkin::Bone
                    {
                        ._inverseBindMatrix = inverseBindMatrices ? (*inverseBindMatrices)[i]
                                                                  : glm::identity<glm::mat4>(), // TODO: maybe mat4(0)?
                        ._jointNode = it->second,
                    });
                }

                // upload to the scene
                scene.handle(events::controller::SceneSetMeshSkin
                {
                    ._item = meshPath,
                    ._attribute = models::MeshSkin
                    {
                        ._origin = origin,
                        ._bones = bones,
                    },
                });
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

        Context context;
        if (path.size() == 1)
        {
            // only name of file specified => load default scene
            MINIRE_INVARIANT(gltf->defaultScene >= 0, "no default scene ({}): {}",
                             gltf->defaultScene, path);
            size_t const defaultScene = static_cast<size_t>(gltf->defaultScene);
            instantiateGltfScene(scene, e, *gltf, defaultScene, context);
        }
        else
        {
            // tail components in a path after a filename => should load some part of glTF file
            MINIRE_INVARIANT(path.size() == 3, "unexpected gLTF path format: {}", path);

            // fetch a kind of a file part to instantiated
            MINIRE_INVARIANT(std::holds_alternative<content::path::Special>(path[1]),
                             "unexpected second component type: {}", path);
            content::path::Special const collection = std::get<content::path::Special>(path[1]);

            switch(collection)
            {
                case content::path::Special::kCameras:
                    instantiateGltfCamera(scene, e, *gltf, path[2]);
                    break;

                case content::path::Special::kLights:
                    instantiateGltfLight(scene, e, *gltf, path[2]);
                    break;

                case content::path::Special::kMeshes:
                    instantiateGltfMesh(scene, e, *gltf, path[2]);
                    break;

                case content::path::Special::kNodes:
                    instantiateGltfNode(scene, e, *gltf, path[2], true, context);
                    break;

                case content::path::Special::kScenes:
                    instantiateGltfScene(scene, e, *gltf, path[2], context);
                    break;

                case content::path::Special::kVertexBuffers:
                    MINIRE_THROW("vertex-buffer cannot be a part of gLTF collection: {}", path);
            }
        }

        attachPendedSkins(scene, *gltf, context);
    }
}
