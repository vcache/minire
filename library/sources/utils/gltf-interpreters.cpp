// TODO: this whole file is a mess, should refactor it

#include <utils/gltf-interpreters.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/models/image.hpp>
#include <minire/models/pbr-material.hpp>
#include <minire/models/sampler.hpp>

#include <utils/uuid.hpp>

#include <algorithm>
#include <initializer_list>
#include <tuple>

// TODO: set "LoadImageDataOption::preserve_channels" to avoid unnecessary components loading

namespace minire::utils
{
    namespace
    {
        // see https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview
        constexpr static std::string kPosition = "POSITION";
        constexpr static std::string kNormal = "NORMAL";
        constexpr static std::string kTangent = "TANGENT";
        constexpr static std::string kTexCoord0 = "TEXCOORD_0";
        // TODO: TEXCOORD_n, COLOR_n, JOINTS_n, and WEIGHTS_n.

        GLenum gltfModeToGlMode(int gltfMode)
        {
            switch(gltfMode)
            {
                case TINYGLTF_MODE_POINTS:          return GL_POINTS;
                case TINYGLTF_MODE_LINE:            return GL_LINES;
                case TINYGLTF_MODE_LINE_LOOP:       return GL_LINE_LOOP;
                case TINYGLTF_MODE_LINE_STRIP:      return GL_LINE_STRIP;
                case TINYGLTF_MODE_TRIANGLES:       return GL_TRIANGLES;
                case TINYGLTF_MODE_TRIANGLE_STRIP:  return GL_TRIANGLE_STRIP;
                case TINYGLTF_MODE_TRIANGLE_FAN:    return GL_TRIANGLE_FAN;
                default: MINIRE_THROW("gltf draw mode isn't supported: {}", gltfMode);
            }
        }

        GLenum gltfComponentTypeToGlType(uint32_t componentType)
        {
            switch(componentType)
            {
                case TINYGLTF_COMPONENT_TYPE_BYTE:              return GL_BYTE;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:     return GL_UNSIGNED_BYTE;
                case TINYGLTF_COMPONENT_TYPE_SHORT:             return GL_SHORT;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:    return GL_UNSIGNED_SHORT;
                case TINYGLTF_COMPONENT_TYPE_INT:               return GL_INT;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:      return GL_UNSIGNED_INT;
                case TINYGLTF_COMPONENT_TYPE_FLOAT:             return GL_FLOAT;
                case TINYGLTF_COMPONENT_TYPE_DOUBLE:            return GL_DOUBLE;
                default: MINIRE_THROW("gltf componentType isn't supported: {}", componentType);
            }
        }

        class RawImage : public models::Image
        {
            // Should extend lifetime of a Model since _data
            // points to the internal buffer of the Model.
            std::shared_ptr<::tinygltf::Model const> _model;

        public:
            explicit RawImage(::tinygltf::Image const & image,
                              std::shared_ptr<::tinygltf::Model> const & model)
                : _model(model)
            {
                MINIRE_INVARIANT(image.width > 0 && image.height > 0,
                                 "bad gLTF image size: {}x{}, {}",
                                 image.width, image.height, image.name);

                _width = image.width;
                _height = image.height;

                switch(image.component)
                {
                    // grey
                    case 1:
                        _format = Format::kGrayscale;
                        break;

                    case 2: // grey, alpha
                        MINIRE_THROW("unsupported channels count: {}", image.component);

                    case 3: // red, green, blue
                        _format = Format::kRGB;
                        break;

                    case 4: // red, green, blue, alpha
                        _format = Format::kRGBA;
                        break;

                    default:
                        MINIRE_THROW("unexpected component count: {}", image.component);
                }

                MINIRE_INVARIANT(image.bits == 8, "unsupported bits for image: {}, {}",
                                 image.bits, image.name);
                _depth = Depth::k8;

                MINIRE_INVARIANT(image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE,
                                 "unexpected pixel_type: {}, {}", image.pixel_type, image.name);
                _signed = false;

                // have to do a const_cast, but should be safe since,
                // not modifying operations will be performed
                _data = const_cast<uint8_t *>(image.image.data());
            }
        };

        size_t requireAttr(::tinygltf::Mesh const & mesh,
                           ::tinygltf::Primitive const & primitive,
                           std::string const & name)
        {
            auto it = primitive.attributes.find(name);
            MINIRE_INVARIANT(it != primitive.attributes.cend(),
                             "no {} index attribute: {}", name, mesh.name);
            MINIRE_INVARIANT(it->second >= 0, "bad {} index: {}, {}",
                                              name, it->second, mesh.name)
            return static_cast<size_t>(it->second);
        }

        ::tinygltf::Accessor const & getAccessor(size_t const index,
                                                 ::tinygltf::Model const & model)
        {
            MINIRE_INVARIANT(index < model.accessors.size(),
                             "no such accessor: {} >= {}", index,
                             model.accessors.size());
            return model.accessors[index];
        }

        Aabb calcAabb(::tinygltf::Accessor const & position,
                      std::string const & tag)
        {
            MINIRE_INVARIANT(position.type == TINYGLTF_TYPE_VEC3,
                             "position isn't Vec3: {}, {}/{}",
                             position.type, tag, position.name);

            std::vector<double> const & min = position.minValues;
            std::vector<double> const & max = position.maxValues;

            MINIRE_INVARIANT(min.size() == 3,
                             "minValues are not 3: {}, {}/{}",
                             min.size(), tag, position.name);

            MINIRE_INVARIANT(max.size() == 3,
                             "maxValues are not 3: {}, {}/{}",
                             max.size(), tag, position.name);

            return Aabb(glm::vec3{min[0], min[1], min[2]},
                        glm::vec3{max[0], max[1], max[2]});
        }

        ::tinygltf::BufferView const & getBufferView(::tinygltf::Accessor const & accessor,
                                                     ::tinygltf::Model const & model)
        {
            MINIRE_INVARIANT(accessor.bufferView >= 0,
                             "bufferView isn't specified (a sparse buffer?): {}, {}",
                             accessor.bufferView, accessor.name);

            size_t const bufferViewIndex = static_cast<size_t>(accessor.bufferView);
            MINIRE_INVARIANT(bufferViewIndex < model.bufferViews.size(),
                             "bad bufferView: {} >= {}, {}",
                             bufferViewIndex, model.bufferViews.size(), accessor.name);

            return model.bufferViews[bufferViewIndex];
        }

        ::tinygltf::Buffer const & getBuffer(::tinygltf::BufferView const & bufferView,
                                             ::tinygltf::Model const & model)
        {
            MINIRE_INVARIANT(bufferView.buffer >= 0,
                             "buffer isn't specified (wtf?): {}", bufferView.buffer);

            size_t const bufferIndex = static_cast<size_t>(bufferView.buffer);
            MINIRE_INVARIANT(bufferIndex < model.buffers.size(),
                             "bad buffer index: {} >= {}", bufferIndex, model.buffers.size());
            return model.buffers[bufferIndex];
        }

        std::tuple<::tinygltf::Accessor const &,
                   ::tinygltf::BufferView const &>
        createVbo(::tinygltf::Model const & model,
                  size_t const accessorIndex,
                  GLenum const target,
                  opengl::VertexBuffer & result)
        {
            ::tinygltf::Accessor const & accessor = getAccessor(accessorIndex, model);

            ::tinygltf::BufferView const & bufferView = getBufferView(accessor, model);
            MINIRE_INVARIANT(bufferView.target >= 0, "bad bufferView.target = {}",
                             bufferView.target);

            MINIRE_INVARIANT(static_cast<GLenum>(bufferView.target) == target,
                             "unexpected VBO target: {} != {}, {}",
                             bufferView.target, target, accessor.name);

            ::tinygltf::Buffer const & buffer = getBuffer(bufferView, model);
            MINIRE_INVARIANT(bufferView.byteOffset + bufferView.byteLength <= buffer.data.size(),
                             "buffer overflow: {}, {}, {}, {}",
                             bufferView.byteOffset, bufferView.byteLength, buffer.data.size(),
                             accessor.name);

            opengl::VBO & vbo = result.createVbo(accessorIndex, target);
            //static_assert(sizeof(typename ::tinygltf::Buffer::data::value_type) == 1,
            //              "data vector's size isn't equal 1 byte");
            vbo.bufferData(bufferView.byteLength,
                           buffer.data.data() + bufferView.byteOffset,
                           GL_STATIC_DRAW);
            return {accessor, bufferView};
        }

        void setupSampler(::tinygltf::Model const & model,
                          ::tinygltf::Texture const & texture,
                          models::Sampler & out)
        {
            if (texture.sampler >= 0)
            {
                size_t const samplerIndex = static_cast<size_t>(texture.sampler);
                MINIRE_INVARIANT(samplerIndex < model.samplers.size(),
                                 "bad sampler index: {} >= {}, {}",
                                 samplerIndex, model.samplers.size(), texture.name);
                ::tinygltf::Sampler const & sampler = model.samplers[samplerIndex];

                if (sampler.minFilter != -1) out._minFilter = sampler.minFilter;
                if (sampler.magFilter != -1) out._magFilter = sampler.magFilter;
                if (sampler.wrapS != -1) out._wrapS = sampler.wrapS;
                if (sampler.wrapT != -1) out._wrapT = sampler.wrapT;
            }
        }

        struct MaterialData
        {
            using Leases = std::vector<std::unique_ptr<content::Lease>>;

            material::Model::Uptr _materialModel;
            Leases                _textureLeases;
        };

        std::pair<content::Id, ::tinygltf::Texture const *>
        fetchTexture(std::shared_ptr<::tinygltf::Model> const & model,
                     int index, int texCoord,
                     content::Manager & contentManager,
                     MaterialData::Leases & leases)
        {
            if (index >= 0)
            {
                assert(model);

                size_t const textureIndex = static_cast<size_t>(index);
                MINIRE_INVARIANT(texCoord == 0, "multiple TEXCOORD's isn't supported");
                MINIRE_INVARIANT(textureIndex < model->textures.size(),
                                 "bad texture index: {} >= {}",
                                 textureIndex, model->textures.size());

                ::tinygltf::Texture const & texture = model->textures[textureIndex];
                MINIRE_INVARIANT(texture.source >= 0, "no source of a texture: {}", texture.name);
                size_t const imageIndex = static_cast<size_t>(texture.source);
                MINIRE_INVARIANT(imageIndex < model->images.size(),
                                 "bad source index: {} >= {}, {}",
                                 imageIndex, model->images.size(), texture.name);

                ::tinygltf::Image const & image = model->images[imageIndex];
                MINIRE_INVARIANT(!image.as_is, "Image of {} isn't preloaded: {}",
                                 texture.name, image.name);

                std::shared_ptr<models::Image> rawImage = std::make_shared<RawImage>(image, model);
                std::string imageId = fmt::format("__minire_gltf_image_{}", newUuid());
                auto lease = contentManager.upload(imageId, std::move(rawImage));
                leases.emplace_back(std::move(lease));

                return std::make_pair(imageId, &texture);
            }
            return std::make_pair(content::Id(), nullptr);
        }

        MaterialData createMaterialModel(std::shared_ptr<::tinygltf::Model> const & model,
                                         ::tinygltf::Material const & material,
                                         content::Manager & contentManager)
        {
            assert(model);

            MINIRE_INVARIANT(material.alphaMode == "OPAQUE",
                             "unsupported alphaMode: \"{}\"", material.alphaMode);
            // NOTE: alphaCutoff is ignored
            MINIRE_INVARIANT(!material.doubleSided, "Double sided materials aren't supported");
            MINIRE_INVARIANT(material.lods.empty(), "MSFT_lod isn't supported");

            /*
            TODO:
                The base color texture MUST contain 8-bit values encoded with the sRGB opto-electronic
                transfer function so RGB values MUST be decoded to real linear values before they are
                used for any computations. To achieve correct filtering, the transfer function SHOULD
                be decoded before performing linear interpolation.
            */

            /*
            TODO:
                In addition to the material properties, if a primitive specifies a vertex color using
                the attribute semantic property COLOR_0, then this value acts as an additional linear
                multiplier to base color.
            */

            models::PbrMaterial result;
            MaterialData::Leases leases;

            ::tinygltf::PbrMetallicRoughness const & pbrMr = material.pbrMetallicRoughness;
            MINIRE_INVARIANT(pbrMr.baseColorFactor.size() == 3,
                             "bad baseColorFactor = {}", pbrMr.baseColorFactor.size());
            result._albedoFactor = glm::vec3{pbrMr.baseColorFactor[0],
                                             pbrMr.baseColorFactor[1],
                                             pbrMr.baseColorFactor[2]};

            if (auto [contentId, texture] = fetchTexture(
                    model, pbrMr.baseColorTexture.index,
                    pbrMr.baseColorTexture.texCoord,
                    contentManager, leases);
                texture)
            {
                assert(!contentId.empty());
                result._albedoTexture = contentId;
                setupSampler(*model, *texture, result._albedoSampler);
            }

            result._metallicFactor = pbrMr.metallicFactor;
            result._roughnessFactor = pbrMr.roughnessFactor;

            if (auto [contentId, texture] = fetchTexture(
                    model, pbrMr.metallicRoughnessTexture.index,
                    pbrMr.metallicRoughnessTexture.texCoord,
                    contentManager, leases);
                texture)
            {
                assert(!contentId.empty());
                result._metallicTexture = contentId;
                result._roughnessTexture = contentId;
                setupSampler(*model, *texture, result._metallicSampler);
                setupSampler(*model, *texture, result._roughnessSampler);
            }

            if (auto [contentId, texture] = fetchTexture(
                    model, material.normalTexture.index,
                    material.normalTexture.texCoord,
                    contentManager, leases);
                texture)
            {
                assert(!contentId.empty());
                result._normalTexture = contentId;
                setupSampler(*model, *texture, result._normalSampler);
                result._normalScale = material.normalTexture.scale;
            }

            if (auto [contentId, texture] = fetchTexture(
                    model, material.occlusionTexture.index,
                    material.occlusionTexture.texCoord,
                    contentManager, leases);
                texture)
            {
                assert(!contentId.empty());
                result._aoTexture = contentId;
                setupSampler(*model, *texture, result._aoSampler);
                result._aoStrength = material.occlusionTexture.strength;
            }

            MINIRE_INVARIANT(material.emissiveFactor.size() == 3,
                             "bad emissiveFactor = {}", material.emissiveFactor.size());
            result._emissiveFactor = glm::vec3(material.emissiveFactor[0],
                                               material.emissiveFactor[1],
                                               material.emissiveFactor[2]);

            if (auto [contentId, texture] = fetchTexture(
                    model, material.emissiveTexture.index,
                    material.emissiveTexture.texCoord,
                    contentManager, leases);
                texture)
            {
                assert(!contentId.empty());
                result._emissiveTexture = contentId;
                setupSampler(*model, *texture, result._emissiveSampler);
            }

            return MaterialData
            {
                std::make_unique<models::PbrMaterial>(std::move(result)),
                std::move(leases),
            };
        }

        // TODO: support sparse buffers
        // TODO: see glDrawArrays, glDrawRangeElements, glMultiDrawElements, or glMultiDrawArrays
        //       for cases w/o indeces and multiple primitives
        // TODO: Client implementations SHOULD support at least two texture coordinate sets, ...
        // TODO: When normals are not specified, client implementations MUST calculate flat normals and
        //       the provided tangents (if present) MUST be ignored.
        // TODO: When tangents are not specified, client implementations SHOULD calculate tangents using
        //       default MikkTSpace algorithms with the specified vertex positions, normals, and texture
        //       coordinates associated with the normal texture.
        // TODO: don't load texture automatically, since they might be controller via content::Manger
        opengl::VertexBuffer createVertexBuffer(::tinygltf::Model const & model,
                                                ::tinygltf::Mesh const & mesh,
                                                ::tinygltf::Primitive const & primitive,
                                                int vtxAttribIndex,
                                                int uvAttribIndx,
                                                int normAttrib,
                                                int tangentAttrib)
        {
            opengl::VertexBuffer result;

            // Elements buffer

            {
                // TODO: When indices property is not defined, the number of vertex indices to render is
                //       defined by count of attribute accessors
                MINIRE_INVARIANT(primitive.indices >= 0, "indices are not specified: {}", mesh.name);
                auto const & [accessor, _] = createVbo(model, static_cast<size_t>(primitive.indices),
                                                       GL_ELEMENT_ARRAY_BUFFER, result);

                MINIRE_INVARIANT(TINYGLTF_TYPE_SCALAR == accessor.type,
                                 "indices are not scalar: {}, {}", accessor.type, mesh.name);

                MINIRE_INVARIANT(0 == accessor.byteOffset,
                                 "byteOffset of indices's accessor isn't zero: {}, {}",
                                 accessor.byteOffset, mesh.name);

                result._elementsCount = accessor.count;
                result._elementsType = gltfComponentTypeToGlType(accessor.componentType);
            }

            // Vertex buffer and attributes

            using Attribs = std::initializer_list<std::tuple<std::string const &, int>>;
            for(auto const & [accessorName, attribIndex] : Attribs {{kPosition, vtxAttribIndex},
                                                                    {kTexCoord0, uvAttribIndx},
                                                                    {kNormal, normAttrib},
                                                                    {kTangent, tangentAttrib}})
            {
                if (attribIndex == -1) continue;

                size_t const accessorIndex = requireAttr(mesh, primitive, accessorName);
                auto const & [accessor, bufferView] = createVbo(model, accessorIndex, GL_ARRAY_BUFFER, result);

                if (accessorName == kPosition)
                {
                    result._aabb = calcAabb(accessor, mesh.name);
                }

                result._vao->enableAttrib(attribIndex);
                result._vao->attribPointer(attribIndex,
                                           ::tinygltf::GetNumComponentsInType(accessor.type),
                                           gltfComponentTypeToGlType(accessor.componentType),
                                           accessor.normalized ? GL_TRUE : GL_FALSE,
                                           bufferView.byteStride, accessor.byteOffset);
            }

            result._drawMode = gltfModeToGlMode(primitive.mode);

            return result;
        }
    }

    // Publicly visible functions

    GltfMeshFeatures prefetchGltfFeatures(std::shared_ptr<::tinygltf::Model> const & model,
                                         size_t const meshIndex, content::Manager & contentManager)
    {
        assert(model);

        // fetch the mesh

        MINIRE_INVARIANT(meshIndex < model->meshes.size(),
                         "mesh doesn't exist: {} >= {}", meshIndex, model->meshes.size());
        ::tinygltf::Mesh const & mesh = model->meshes[meshIndex];

        // init resulting structure

        GltfMeshFeatures result;
        result._materialModels.resize(model->materials.size());
        result._primitives.reserve(mesh.primitives.size());
        result._textureLeases.reserve(model->materials.size() * 6);

        // iterate primitives

        for(size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex)
        {
            ::tinygltf::Primitive const & primitive = mesh.primitives[primitiveIndex];

            models::MeshFeatures meshFeatures(primitive.attributes.contains(kTexCoord0),
                                              primitive.attributes.contains(kNormal),
                                              primitive.attributes.contains(kTangent));

            bool const hasMaterial = primitive.material >= 0;
            size_t materialIndex = hasMaterial ? static_cast<size_t>(primitive.material)
                                               : GltfMeshFeatures::kNoIndex;
            result._primitives.emplace_back(GltfMeshFeatures::Primitive{meshFeatures, materialIndex});

            MINIRE_INVARIANT(!hasMaterial || materialIndex < result._materialModels.size(),
                             "bad material index: {} >= {}",
                             materialIndex, result._materialModels.size());
            assert(result._materialModels.size() == model->materials.size());
            if (hasMaterial && !result._materialModels[materialIndex])
            {
                ::tinygltf::Material const & material = model->materials[materialIndex];
                MaterialData materialData = createMaterialModel(model, material, contentManager);
                result._materialModels[materialIndex] = std::move(materialData._materialModel);
                std::move(materialData._textureLeases.begin(),
                          materialData._textureLeases.end(),
                          std::back_inserter(result._textureLeases));
            }
        }

        return result;
    }

    std::vector<opengl::VertexBuffer>
    createVertexBuffers(::tinygltf::Model const & model,
                        size_t const meshIndex,
                        std::vector<material::Program::Locations> const & locationsForPrims)
    {
        // fetch the mesh

        MINIRE_INVARIANT(meshIndex < model.meshes.size(),
                         "mesh doesn't exist: {} >= {}", meshIndex, model.meshes.size());
        ::tinygltf::Mesh const & mesh = model.meshes[meshIndex];

        // initialize the result

        std::vector<opengl::VertexBuffer> result;
        result.reserve(mesh.primitives.size());
        assert(locationsForPrims.size() == mesh.primitives.size());

        // iterate primitives

        for(size_t primitiveIndex = 0; primitiveIndex < mesh.primitives.size(); ++primitiveIndex)
        {
            ::tinygltf::Primitive const & primitive = mesh.primitives[primitiveIndex];
            material::Program::Locations const & locations = locationsForPrims[primitiveIndex];
            result.emplace_back(createVertexBuffer(model, mesh, primitive,
                                                   locations._vertexAttribute,
                                                   locations._uvAttribute,
                                                   locations._normalAttribute,
                                                   locations._tangentAttribute));
        }

        return result;
    }
}
