#pragma once

#include <minire/scene/spatial-index.hpp>

namespace minire::scene
{
    // RAII-style controller over IndexableId
    class SpatialHandler
    {
        SpatialHandler(SpatialHandler const &) = delete;
        SpatialHandler& operator=(SpatialHandler const &) = delete;

    public:
        explicit SpatialHandler(SpatialIndex & spatialIndex,
                                IndexPayload * payload,
                                IndexLayer indexLayer)
            : _spatialIndex(&spatialIndex)
            , _indexableId(_spatialIndex->create(payload, indexLayer))
        {
            assert(payload);
        }

        ~SpatialHandler()
        {
            if (_indexableId != kNoIndexableId && _spatialIndex)
            {
                _spatialIndex->erase(_indexableId);
            }
        }

        SpatialHandler(SpatialHandler && other)
            : _spatialIndex(other._spatialIndex)
            , _indexableId(other._indexableId)
        {
            other._spatialIndex = nullptr;
            other._indexableId = kNoIndexableId;
        }

        SpatialHandler& operator=(SpatialHandler && other)
        {
            SpatialHandler tmp(std::move(other));
            std::swap(_spatialIndex, tmp._spatialIndex);
            std::swap(_indexableId, tmp._indexableId);
            return *this;
        }

    public:
        void update(utils::Aabb const & aabb)
        {
            assert(_spatialIndex);
            assert(_indexableId != kNoIndexableId);
            _spatialIndex->update(_indexableId, aabb);
        }

    private:
        SpatialIndex * _spatialIndex = nullptr;
        IndexableId    _indexableId = kNoIndexableId;
    };
}