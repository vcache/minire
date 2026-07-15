#include <minire/scene/spatial-index/brute-force.hpp>

namespace minire::scene::spatial_index
{
    void BruteForce::cull(utils::FrustumPlanes const &,
                          IndexLayer indexLayer,
                          std::vector<IndexPayload *> & output) const
    {
        iterate(indexLayer,
                [&output] (IndexableId, IndexPayload * payload, IndexLayer)
                {
                    output.emplace_back(payload);
                });
    }

    void BruteForce::createImpl(IndexableId, utils::Aabb const &) {}
    void BruteForce::updateImpl(IndexableId, utils::Aabb const &) {}
    void BruteForce::eraseImpl(IndexableId) {}
}
