#include <utils/obj-interpreters.hpp>

#include <minire/formats/obj.hpp>
#include <minire/logging.hpp>
#include <utils/raii-malloc.hpp>

#include <opengl/shared-vertex-buffer.hpp>

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
                size_t result = 0;
                boost::hash_combine(result, std::hash<uint32_t>{}(std::get<0>(k)));
                boost::hash_combine(result, std::hash<uint32_t>{}(std::get<1>(k)));
                boost::hash_combine(result, std::hash<uint32_t>{}(std::get<2>(k)));
                return result;
            }
        };
    }

    models::MeshFeatures getMeshFeatures(formats::Obj const & obj)
    {
        return models::MeshFeatures(obj.haveUvs(), obj.haveNormals(), false, false);
    }

    std::unique_ptr<opengl::SharedVertexSubBuffer>
    createVertexBuffer(formats::Obj const & mesh,
                       material::Locations const & /*locations*/,
                       opengl::SharedVertexBuffer & svb)
    {
        Aabb aabb;
        size_t const stride = svb.stride();
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

        MINIRE_DEBUG("OBJ to VertexBuffer cache hit rate: {}%",
                     static_cast<float>(hits) / static_cast<float>(elements.size()) * 100.0f);

        // Create the buffers
        assert(svb.drawMode() == GL_TRIANGLES);
        assert(svb.elementsType() == GL_UNSIGNED_INT);

        return svb.create(
            elements.size(), aabb,                          // general data
            elements.bytesCount(), elements.bytesPointer(), // EBO
            attribs.size() * sizeof(float), attribs.data()  // VBO
        );
    }
}
