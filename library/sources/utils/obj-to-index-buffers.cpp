#include <utils/obj-to-index-buffers.hpp>

#include <minire/formats/obj.hpp>
#include <minire/logging.hpp>
#include <utils/raii-malloc.hpp>

#include <boost/container_hash/hash.hpp> // for hash

#include <unordered_map>
#include <cassert>
#include <tuple>

namespace minire::utils
{
    namespace
    {
        using FaceKey = std::tuple<uint32_t, uint32_t, uint32_t>;

        struct FaceKeyHash
        {
            size_t operator()(FaceKey const & k) const
            {
                size_t result = 0x825734fB40AEC515ULL;
                boost::hash_combine(result, std::hash<uint32_t>{}(std::get<0>(k)));
                boost::hash_combine(result, std::hash<uint32_t>{}(std::get<1>(k)));
                boost::hash_combine(result, std::hash<uint32_t>{}(std::get<2>(k)));
                return result;
            }
        };

        size_t strideOf(formats::Obj const & mesh)
        {
            size_t result = 3;
            if (mesh.haveUvs()) result += 2;
            if (mesh.haveNormals()) result += 3;
            return result;
        }
    }

    opengl::IndexBuffers createIndexBuffers(formats::Obj const & mesh,
                                            int vtxAttribIndex,
                                            int uvAttribIndx,
                                            int normAttrib)
    {
        Aabb aabb;
        size_t const stride = strideOf(mesh);
        RaiiMalloc<uint32_t> elements(mesh._faceVertices.size());

        std::unordered_map<FaceKey, uint32_t, FaceKeyHash> allocCache;

        // attribute structure: (x, y, z) [u, v] [nx, ny, nz]
        std::vector<float> attribs;
        attribs.reserve(stride * mesh._vertices.size());

        // prepare buffers
        size_t hits = 0;
        for(size_t i(0); i < elements.size(); ++i)
        {
            FaceKey faceKey(mesh._faceVertices[i],
                            mesh.haveUvs() ? mesh._faceUvs[i] : 0,
                            mesh.haveNormals() ? mesh._faceNormals[i] : 0);
            auto const it = allocCache.find(faceKey);
            if (it != allocCache.cend())
            {
                elements[i] = it->second;
                ++hits;
                continue;
            }

            {
                glm::vec3 const & vertex = mesh._vertices[std::get<0>(faceKey)];
                attribs.push_back(vertex.x);
                attribs.push_back(vertex.y);
                attribs.push_back(vertex.z);
                aabb.extend(vertex);
            }

            if (mesh.haveUvs())
            {
                glm::vec2 const & uv = mesh._uvs[std::get<1>(faceKey)];
                attribs.push_back(uv.x);
                attribs.push_back(uv.y);
            }

            if (mesh.haveNormals())
            {
                glm::vec3 const & normal = mesh._normals[std::get<2>(faceKey)];
                attribs.push_back(normal.x);
                attribs.push_back(normal.y);
                attribs.push_back(normal.z);
            }

            assert(0 == (attribs.size() % stride));
            uint32_t const index = (attribs.size() / stride) - 1;
            elements[i] = index;
            allocCache.emplace(faceKey, index);
        }

        MINIRE_DEBUG("OBJ to IndexBuffers cache hit rate: {}%",
                     static_cast<float>(hits) / static_cast<float>(elements.size()) * 100.0f);

        // Create the  buffers
        opengl::IndexBuffers result(elements.size(), aabb);

        // load EBO and VBO
        result._ebo.bufferData(elements.bytesCount(), elements.bytesPointer(), GL_STATIC_DRAW);
        result._vbo.bufferData(attribs.size() * sizeof(float), attribs.data(), GL_STATIC_DRAW);

        size_t pointer = 0;
        size_t const bstride = stride * sizeof(float);

        // setup VAO
        result._vao->enableAttrib(vtxAttribIndex);
        result._vao->attribPointer(vtxAttribIndex, 3, GL_FLOAT, GL_FALSE, bstride, pointer);
        pointer += (3 * sizeof(float));

        if (mesh.haveUvs())
        {
            result._vao->enableAttrib(uvAttribIndx);
            result._vao->attribPointer(uvAttribIndx, 2, GL_FLOAT, GL_FALSE, bstride, pointer);
            pointer += (2 * sizeof(float));
            result._flags |= opengl::IndexBuffers::kHaveUvs;
        }

        if (mesh.haveNormals())
        {
            result._vao->enableAttrib(normAttrib);
            result._vao->attribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, bstride, pointer);
            pointer += (3 * sizeof(float)); // TODO: useless
            result._flags |= opengl::IndexBuffers::kHaveNormals;
        }

        return result;
    }
}
