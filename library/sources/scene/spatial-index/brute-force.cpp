#include <minire/scene/spatial-index/brute-force.hpp>

namespace minire::scene::spatial_index
{
    void BruteForce::cull(utils::FrustumPlanes const &,
                          IndexLayer targetIndexLayer,
                          std::vector<IndexPayload *> & output) const
    {
        iterate([&output, targetIndexLayer]
                (IndexableId, IndexPayload * payload, IndexLayer indexLayer)
                {
                    if (indexLayer == targetIndexLayer)
                        output.emplace_back(payload);
                });
    }

    void BruteForce::createImpl(IndexableId, utils::Aabb const &) {}
    void BruteForce::updateImpl(IndexableId, utils::Aabb const &) {}
    void BruteForce::eraseImpl(IndexableId) {}
}