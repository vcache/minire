#include <rasterizer/vertex-buffers.hpp>

#include <minire/errors.hpp>
#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/models/vertex-buffer.hpp>

#include <opengl/vertex-buffer.hpp>
#include <rasterizer/resources.hpp>
#include <utils/vertex-buffer-builder.hpp>

#include <cassert>
#include <unordered_map>

namespace minire::rasterizer
{
    namespace
    {
        models::MeshFeatures inferMeshFeatures(models::VertexBuffer const & vertexBuffer)
        {
            bool const hasUv = !std::holds_alternative<std::monostate>(vertexBuffer._uvs);
            bool const hasNormal = !std::holds_alternative<std::monostate>(vertexBuffer._normals);
            bool const hasTangent = !std::holds_alternative<std::monostate>(vertexBuffer._tangents);
            return models::MeshFeatures(hasUv, hasNormal, hasTangent, false);
        }

        struct VertexBufferData
        {
            using GpuInstances = std::unordered_map<material::Program::Locations,
                                                    std::shared_ptr<opengl::VertexBuffer>>;

            models::VertexBuffer _vertexBuffer;
            models::MeshFeatures _meshFeatures;
            GpuInstances         _gpuInstances;

            using Sptr = std::shared_ptr<VertexBufferData>;

            VertexBufferData(models::VertexBuffer const & vertexBuffer)
                : _vertexBuffer(vertexBuffer)
                , _meshFeatures(inferMeshFeatures(_vertexBuffer))
            {}

            void update(models::VertexBuffer const & vertexBuffer)
            {
                models::MeshFeatures newMeshFeatures = inferMeshFeatures(vertexBuffer);
                MINIRE_INVARIANT(newMeshFeatures == _meshFeatures,
                                 "mesh features of a new vertex-buffer differs from existing");
                _vertexBuffer = vertexBuffer;
                _meshFeatures = newMeshFeatures;
                for(auto & [locations, instance] : _gpuInstances)
                {
                    assert(instance);
                    auto newInstance = utils::createVertexBuffer(_vertexBuffer, locations);
                    assert(newInstance);
                    *instance = std::move(*newInstance);
                }
            }

            std::shared_ptr<opengl::VertexBuffer>
            build(material::Program::Locations const & locations)
            {
                auto it = _gpuInstances.find(locations);
                if(it == _gpuInstances.end())
                {
                    auto gpuVertexBuffer = utils::createVertexBuffer(_vertexBuffer, locations);
                    auto [newIt, inserted] = _gpuInstances.emplace(locations, gpuVertexBuffer);
                    MINIRE_INVARIANT(inserted, "failed to insert new vertex-buffer");
                    it = newIt;
                }
                assert(it != _gpuInstances.end());
                return it->second;
            }
        };

        VertexBufferData::Sptr find(content::Id const & id,
                                    Resources const & resources)
        {
            std::any cached = resources.find(vertex_buffers::Id{id});
            MINIRE_INVARIANT(cached.has_value(), "no such vertex-buffer: {}", id);
            auto const data = std::any_cast<VertexBufferData::Sptr>(cached);
            assert(data);
            return data;
        }
    }

    // VertexBuffers

    VertexBuffers::VertexBuffers(Resources & resources)
        : _resources(resources)
    {}

    void VertexBuffers::create(content::Id const & id,
                               models::VertexBuffer const & vertexBuffer,
                               bool const override)
    {
        // NOTE: Cannot just call Resources::insert(..., ..., true),
        //       because it will destroy existing shared pointers to
        //       opengl::VertexBuffer. Therefore, override operation
        //       must be performed in-place.

        vertex_buffers::Id resourceKey{id};
        if (std::any cached = _resources.find(resourceKey);
            cached.has_value())
        {
            if (!override)
            {
                MINIRE_THROW("a vertex-buffer is already exists and 'override' didn't set: {}", id);
            }

            auto data = std::any_cast<VertexBufferData::Sptr>(cached);
            assert(data);
            data->update(vertexBuffer);
        }
        else
        {
            auto data = std::make_shared<VertexBufferData>(vertexBuffer);
            _resources.insert(resourceKey, std::move(data));
        }
    }

    void VertexBuffers::dispose(content::Id const & id)
    {
        _resources.erase(vertex_buffers::Id{id});
    }

    models::MeshFeatures VertexBuffers::meshFeatures(content::Id const & id) const
    {
        return find(id, _resources)->_meshFeatures;
    }

    std::shared_ptr<opengl::VertexBuffer>
    VertexBuffers::build(content::Id const & id,
                         material::Program::Locations const & locations) const
    {
        return find(id, _resources)->build(locations);
    }
}
