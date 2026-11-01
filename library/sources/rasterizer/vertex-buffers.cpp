#include <rasterizer/vertex-buffers.hpp>

#include <minire/errors.hpp>
#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/models/vertex-buffer.hpp>

#include <opengl/shared-vertex-buffer.hpp>
#include <rasterizer/resources.hpp>
#include <rasterizer/shared-vertex-buffers.hpp>
#include <utils/vertex-buffer-builder.hpp>

#include <cassert>

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
            models::VertexBuffer                           _vertexBuffer;
            models::MeshFeatures                           _meshFeatures;
            std::shared_ptr<opengl::SharedVertexSubBuffer> _subBuffer;

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
                if (_subBuffer)
                {
                    utils::updateVertexSubBuffer(_vertexBuffer, *_subBuffer);
                }
            }

            std::shared_ptr<opengl::SharedVertexSubBuffer>
            build(SharedVertexBuffers & sharedVertexBuffers)
            {
                if (!_subBuffer)
                {
                    GLenum const drawMode = utils::convertDrawMode(_vertexBuffer._drawMode);
                    opengl::SharedVertexBuffer::Sptr const & svb =
                        sharedVertexBuffers.get(_meshFeatures, drawMode);
                    assert(svb);
                    _subBuffer = utils::createVertexSubBuffer(_vertexBuffer, *svb);
                }
                return _subBuffer;
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
        auto tmp = find(id, _resources);
        return tmp->_meshFeatures;
    }

    std::shared_ptr<opengl::SharedVertexSubBuffer>
    VertexBuffers::build(content::Id const & id,
                         SharedVertexBuffers & sharedVertexBuffers) const
    {
        auto tmp = find(id, _resources);
        return tmp->build(sharedVertexBuffers);
    }
}
