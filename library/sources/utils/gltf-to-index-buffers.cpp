// TODO: this whole file is a mess, should refactor it

#include <utils/gltf-to-index-buffers.hpp>

#include <minire/errors.hpp>

#include <initializer_list>
#include <tuple>

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
    }

    // TODO: support sparse buffers
    // TODO: support multiple primitives
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
                                            size_t const meshIndex,
                                            int vtxAttribIndex,
                                            int uvAttribIndx,
                                            int normAttrib,
                                            int tangentAttrib)
    {
        opengl::VertexBuffer result;

        // fetch the mesh

        MINIRE_INVARIANT(meshIndex < model.meshes.size(),
                         "mesh doesn't exist: {} >= {}", meshIndex, model.meshes.size());
        ::tinygltf::Mesh const & mesh = model.meshes[meshIndex];

        // fetch a primitive

        MINIRE_INVARIANT(mesh.primitives.size() == 1,
                         "multiple primitives are not supported: {}",
                         mesh.name);
        ::tinygltf::Primitive const & primitive = mesh.primitives[0];

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

        for(auto const & [accessorName, attribIndex, flag] :
            std::initializer_list<std::tuple<std::string const &, int, size_t>> {
                {kPosition, vtxAttribIndex, 0},
                {kTexCoord0, uvAttribIndx, opengl::VertexBuffer::kHaveUvs},
                {kNormal, normAttrib, opengl::VertexBuffer::kHaveNormals},
                {kTangent, tangentAttrib, opengl::VertexBuffer::kHaveTangents}
            })
        {
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
            result._flags |= flag;
        }

        result._drawMode = gltfModeToGlMode(primitive.mode);

        return result;
    }
}
