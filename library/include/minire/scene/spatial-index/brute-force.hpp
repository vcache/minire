#pragma once

#include <minire/scene/spatial-index.hpp>

namespace minire::scene::spatial_index
{
    class BruteForce
        : public SpatialIndex
    {
    public:
        void cull(utils::FrustumPlanes const &,
                  IndexLayer indexLayer,
                  std::vector<IndexPayload *> & output) const override;

    private:
        void createImpl(IndexableId, utils::Aabb const &) override;
        void updateImpl(IndexableId, utils::Aabb const &) override;
        void eraseImpl(IndexableId) override;

    private:
        std::vector<IndexableId> _store;
    };
}