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
#include <minire/scene.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/uuid.hpp>

#include <utils/gltf-buffer-reader.hpp>
#include <utils/gltf-element-fetch.hpp>
#include <utils/gltf-node-transform.hpp>
#include <utils/overloaded.hpp>

#include <fmt/ranges.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <limits>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace minire::scene
{
    namespace
    {
        static constexpr size_t kNoIndex = std::numeric_limits<size_t>::max();

        struct Context
        {
            using BufferData = std::variant<
                std::shared_ptr<std::vector<float> const>,
                std::shared_ptr<std::vector<glm::vec3> const>,
                std::shared_ptr<std::vector<glm::mat4> const>,
                std::shared_ptr<std::vector<glm::quat> const>
            >;

            struct PendedMesh
            {
                scene::Node & _parent;
                std::string  _name;
                models::Mesh _model;
                size_t       _skinIndex; // in the gLTF model
            };

            using NodeIndexToNode = std::unordered_map<size_t /* node index */,
                                                       scene::Node::Sptr>;
            using BufferDataCache = std::unordered_map<size_t /* accessor index */,
                                                       BufferData>;
            using PendedMeshes = std::vector<PendedMesh>;

            NodeIndexToNode _nodeIndexToNode;
            BufferDataCache _bufferDataCache;
            PendedMeshes    _pendedMeshes;
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

        void instantiateGltfCamera(scene::Node & parent,
                                   ::tinygltf::Model const & model,
                                   content::path::Component const & cameraId,
                                   bool const visible)
        {
            size_t const cameraIndex = utils::getElementIndex(cameraId, model.cameras, "camera");
            ::tinygltf::Camera const & camera = model.cameras[cameraIndex];

            if (camera.type == "perspective")
            {
                ::tinygltf::PerspectiveCamera const & pcamera = camera.perspective;
                MINIRE_INVARIANT(pcamera.yfov > 0.0, "bad camera.yfov ({})", pcamera.yfov);
                MINIRE_INVARIANT(pcamera.znear > 0.0, "bad camera.znear ({})", pcamera.znear);
                parent.make(
                    camera.name.empty() ? utils::newUuid() : camera.name,
                    models::PerspectiveCamera
                    {
                        ._yFov = static_cast<float>(pcamera.yfov),
                        ._zNear = static_cast<float>(pcamera.znear),
                        ._zFar = pcamera.zfar > 0.0 ? std::optional<float>(pcamera.zfar)
                                                    : std::nullopt,
                        ._aspectRatio = pcamera.aspectRatio > 0.0 ? std::optional<float>(pcamera.aspectRatio)
                                                                  : std::nullopt,
                        ._visible = visible,
                    });
            }
            else if (camera.type == "orthographic")
            {
                ::tinygltf::OrthographicCamera const & ocamera = camera.orthographic;
                MINIRE_INVARIANT(ocamera.xmag != 0.0, "bad camera.xmag ({})", ocamera.xmag);
                MINIRE_INVARIANT(ocamera.ymag != 0.0, "bad camera.ymag ({})", ocamera.ymag);
                MINIRE_INVARIANT(ocamera.zfar > ocamera.znear, "zfar <= znear ({} <= {})",
                                 ocamera.zfar, ocamera.znear);
                parent.make(
                    camera.name.empty() ? utils::newUuid() : camera.name,
                    models::OrthographicCamera
                    {
                        ._xMag = static_cast<float>(ocamera.xmag),
                        ._yMag = static_cast<float>(ocamera.ymag),
                        ._zNear = static_cast<float>(ocamera.znear),
                        ._zFar = static_cast<float>(ocamera.zfar),
                        ._visible = visible,
                    });
            }
            else
            {
                MINIRE_THROW("unexpected camera type \"{}\"", camera.type);
            }
        }

        size_t instantiateGltfMesh(scene::Node & parent,
                                   ::tinygltf::Model const & model,
                                   content::Id const & sourceId,
                                   content::path::Component const & meshId,
                                   Context & context,
                                   bool const visible)
        {
            size_t const meshIndex = utils::getElementIndex(meshId, model.meshes, "mesh");
            ::tinygltf::Mesh const & mesh = model.meshes[meshIndex];
            context._pendedMeshes.emplace_back(Context::PendedMesh
                {
                    ._parent = parent,
                    ._name = mesh.name.empty() ? utils::newUuid() : mesh.name,
                    ._model = models::Mesh
                    {
                        ._source = content::mkPath(sourceId,
                                                   content::path::Special::kMeshes,
                                                   meshId),
                        ._defaultMaterial = {},
                        ._skin = {}, // will be (maybe) updated in instantiatePendedMeshes()
                        ._visible = visible,
                    },
                    ._skinIndex = kNoIndex,
                });
            return context._pendedMeshes.size() - 1;
        }

        // see ref at:
        //     https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_lights_punctual
        void instantiateGltfLight(scene::Node & parent,
                                  ::tinygltf::Model const & model,
                                  content::path::Component const & lightId,
                                  bool const visible)
        {
            size_t const lightIndex = utils::getElementIndex(lightId, model.lights, "camera");
            ::tinygltf::Light const & light = model.lights[lightIndex];

            if (light.type == "point")
            {
                models::PointLight pointLight(glm::vec4(1.0f, 1.0f, 1.0f, light.intensity),
                                              light.range != 0.0 ? light.range : 3250, // TODO: default range must be infinite
                                              std::nullopt, visible); // TODO: pass shadow params
                if (!light.color.empty())
                {
                    MINIRE_INVARIANT(light.color.size() == 3,
                                     "expected 3 color components, but got {}",
                                     light.color.size());
                    pointLight._color[0] = light.color[0];
                    pointLight._color[1] = light.color[1];
                    pointLight._color[2] = light.color[2];
                }

                parent.make(light.name.empty() ? utils::newUuid() : light.name, pointLight);
            }
            else // TODO: support directional light
            {
                MINIRE_THROW("unsupported light type: \"{}\"", light.type);
            }
        }

        void instantiateGltfNode(scene::Node & parent,
                                 content::Id const & sourceId,
                                 ::tinygltf::Model const & model,
                                 content::path::Component const & nodeId,
                                 bool loadAnimations,
                                 Context & context,
                                 bool const visible)
        {
            //  check preconditions
            size_t const nodeIndex = utils::getElementIndex(nodeId, model.nodes, "node");
            ::tinygltf::Node const & node = model.nodes[nodeIndex];

            MINIRE_INVARIANT(node.lods.empty(), "MSFT_lod isn't supported");
            if (node.emitter >= 0)
            {
                MINIRE_WARNING("audio emitters won't be loaded");
            }

            // create a node itself

            models::Transform transform = utils::getNodeTransform(node);

            scene::Node::Sptr const & newNode = parent.make(
                node.name.empty() ? utils::newUuid() : node.name,
                models::Node
                {
                    ._origin = transform,
                    ._visible = visible,
                });
            {
                auto [_, inserted] = context._nodeIndexToNode.emplace(nodeIndex, newNode);
                MINIRE_INVARIANT(inserted, "broken nodes network ({})", nodeIndex);
            }

            // fill it with leafs

            if (node.camera >= 0)
            {
                instantiateGltfCamera(*newNode, model, static_cast<size_t>(node.camera), true);
            }

            size_t meshIndex = kNoIndex;
            if (node.mesh >= 0)
            {
                meshIndex = instantiateGltfMesh(*newNode, model, sourceId, static_cast<size_t>(node.mesh), context, true);
            }

            if (node.light >= 0)
            {
                instantiateGltfLight(*newNode, model, static_cast<size_t>(node.light), true);
            }

            if (node.skin >= 0)
            {
                MINIRE_INVARIANT(node.mesh >= 0, "cannot have a Skin without a Mesh");
                MINIRE_INVARIANT(meshIndex != kNoIndex, "expected to have a valid mesh instance");
                context._pendedMeshes[meshIndex]._skinIndex = node.skin;
            }

            // fill it with subnode

            for(int subNodeIndex : node.children)
            {
                MINIRE_INVARIANT(subNodeIndex >= 0, "bad sub-node index ({})", subNodeIndex);
                instantiateGltfNode(*newNode, sourceId, model, static_cast<size_t>(subNodeIndex),
                                    false, context, true);
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
                            context._nodeIndexToNode.contains(channel.target_node))
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
                        assert(context._nodeIndexToNode.contains(channel.target_node));
                        scene::Node::Sptr targetNode = context._nodeIndexToNode.at(channel.target_node);
                        MINIRE_INVARIANT(targetNode, "no ScenePath mapping for {}: {}",
                                         channel.target_node, animation.name);

                        // fetch AnimationSampler
                        MINIRE_INVARIANT(channel.sampler >= 0, "bad animation sampler: {}, {}",
                                         channel.sampler, animation.name);
                        size_t const samplerIndex = static_cast<size_t>(channel.sampler);
                        MINIRE_INVARIANT(samplerIndex < animation.samplers.size(),
                                         "bad animation sampler: {} >= {}, {}",
                                         samplerIndex, animation.samplers.size(), animation.name);
                        ::tinygltf::AnimationSampler const & animationSampler = animation.samplers[samplerIndex];

                        // build animation tracks
                        models::KeyframeAnimation & keyframeAnimation = animationTracks[models::NodePointer(targetNode)];
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
                newNode->makeAnimationSet(std::move(animationSet));
            }
        }

        void instantiateGltfScene(scene::Node & parent,
                                  content::Id const & sourceId,
                                  ::tinygltf::Model const & model,
                                  content::path::Component const & sceneId,
                                  Context & context,
                                  bool const visible)
        {
            size_t const sceneIndex = utils::getElementIndex(sceneId, model.scenes, "scene");
            ::tinygltf::Scene const & gltfScene = model.scenes[sceneIndex];

            if (!gltfScene.audioEmitters.empty())
            {
                MINIRE_WARNING("{} audio emitters won't be loaded",
                               gltfScene.audioEmitters.size());
            }

            for(int nodeIndex : gltfScene.nodes)
            {
                MINIRE_INVARIANT(nodeIndex >= 0, "bad node index ({})", nodeIndex);
                instantiateGltfNode(parent, sourceId, model, static_cast<size_t>(nodeIndex), true, context, visible);
            }
        }

        void instantiatePendedMeshes(::tinygltf::Model const & model, Context & context)
        {
            for (Context::PendedMesh & pendedMesh : context._pendedMeshes)
            {
                if (pendedMesh._skinIndex != kNoIndex)
                {
                    // fetch the skin data
                    MINIRE_INVARIANT(pendedMesh._skinIndex < model.skins.size(), "bad skin index ({} >= {})",
                                     pendedMesh._skinIndex, model.skins.size());
                    ::tinygltf::Skin const & skin = model.skins[pendedMesh._skinIndex];

                    // find a 'skeleton' node (if any)
                    scene::Node::Sptr origin;
                    if (skin.skeleton >= 0)
                    {
                        auto it = context._nodeIndexToNode.find(static_cast<size_t>(skin.skeleton));
                        MINIRE_INVARIANT(it != context._nodeIndexToNode.cend(),
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
                    models::Mesh::Skin::Bones bones;
                    bones.reserve(skin.joints.size());
                    for(size_t i = 0; i < skin.joints.size(); ++i)
                    {
                        MINIRE_INVARIANT(skin.joints[i] >= 0, "bad joint index: #{}: {}",
                                         i, skin.joints[i]);
                        size_t const jointIndex = static_cast<size_t>(skin.joints[i]);
                        auto it = context._nodeIndexToNode.find(jointIndex);
                        MINIRE_INVARIANT(it != context._nodeIndexToNode.cend(),
                                         "joint node (id = {}) of Skin \"{}\" isn't instantiated",
                                         jointIndex, skin.name);

                        // NOTE: it is essential to keep bones order as in skins.joints!
                        bones.emplace_back(models::Mesh::Skin::Bone
                        {
                            ._inverseBindMatrix = inverseBindMatrices ? (*inverseBindMatrices)[i]
                                                                      : glm::identity<glm::mat4>(), // TODO: maybe mat4(0)?
                            ._jointNode = it->second,
                        });
                    }

                    // patch model's skin
                    pendedMesh._model._skin = models::Mesh::Skin
                    {
                        ._origin = origin,
                        ._bones = bones,
                    };
                }

                // instantiate a mesh
                pendedMesh._parent.make(pendedMesh._name, std::move(pendedMesh._model));
            }
        }

        void instantiateGltfImpl(scene::Node & parent,
                                 content::Path const & source,
                                 content::Manager & contentManager,
                                 bool visible)
        {
            // Precheck the path and fetch glTF file

            MINIRE_INVARIANT(!source.empty(), "no content path provided");
            MINIRE_INVARIANT(std::holds_alternative<content::Id>(source[0]),
                             "path isn't started from Id: {}", source);
            content::Id const & sourceId = std::get<content::Id>(source[0]);
            auto lease = contentManager.borrow(sourceId);
            assert(lease);

            formats::GltfModelSptr gltf = lease->as<formats::GltfModelSptr>();
            assert(gltf);

            // Build scene graph from a requested part of a glTF file

            Context context;
            if (source.size() == 1)
            {
                // only name of file specified => load default scene
                MINIRE_INVARIANT(gltf->defaultScene >= 0, "no default scene ({}): {}",
                                 gltf->defaultScene, source);
                size_t const defaultScene = static_cast<size_t>(gltf->defaultScene);
                instantiateGltfScene(parent, sourceId, *gltf, defaultScene, context, visible);
            }
            else
            {
                // tail components in a path after a filename => should load some part of glTF file
                MINIRE_INVARIANT(source.size() == 3, "unexpected gLTF path format: {}", source);

                // fetch a kind of a file part to instantiated
                MINIRE_INVARIANT(std::holds_alternative<content::path::Special>(source[1]),
                                 "unexpected second component type: {}", source);
                content::path::Special const collection = std::get<content::path::Special>(source[1]);

                switch(collection)
                {
                    case content::path::Special::kCameras:
                        instantiateGltfCamera(parent, *gltf, source[2], visible);
                        break;

                    case content::path::Special::kLights:
                        instantiateGltfLight(parent,  *gltf, source[2], visible);
                        break;

                    case content::path::Special::kMeshes:
                        instantiateGltfMesh(parent, *gltf, sourceId, source[2], context, visible);
                        break;

                    case content::path::Special::kNodes:
                        instantiateGltfNode(parent, sourceId, *gltf, source[2], true, context, visible);
                        break;

                    case content::path::Special::kScenes:
                        instantiateGltfScene(parent, sourceId, *gltf, source[2], context, visible);
                        break;

                    case content::path::Special::kVertexBuffers:
                        MINIRE_THROW("vertex-buffer cannot be a part of gLTF collection: {}", source);
                }
            }

            instantiatePendedMeshes(*gltf, context);
        }
    }

    void instantiateGltf(scene::Node & parent,
                         content::Path const & source,
                         content::Manager & contentManager,
                         bool visible)
    try
    {
        return instantiateGltfImpl(parent, source, contentManager, visible);
    }
    catch(std::exception const & e)
    {
        MINIRE_THROW("failed to instantiate \"{}\":\n{}", source, e.what());
    }
    catch(...)
    {
        MINIRE_THROW("failed to instantiate \"{}\": (unknown)", source);
    }
}
