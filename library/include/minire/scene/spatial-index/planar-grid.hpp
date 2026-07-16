#pragma once

#include <glm/vec2.hpp>
#include <minire/scene/spatial-index.hpp>
#include <minire/utils/aabb.hpp>

#include <memory>
#include <vector>

namespace minire::scene::spatial_index
{
    // Implements "regular grid" kind index (all-constant operations).
    // The grid is 2D, XZ-plane (Y is ignored!). Will only grow in size.
    // Usefull for top-down games with flat map like RTS, Tycoons, etc
    // The index ca only grow (TODO: implement auto-shrink)
    class PlanarGrid
        : public SpatialIndex
    {
    public:
        using Uptr = std::unique_ptr<PlanarGrid>;

        explicit PlanarGrid(glm::vec2 const & = glm::vec2(5, 5));

        // Note that output list will be deduplicated, that is,
        // every payload pointer will appear just once, even if
        // was inserted several times (under different IndexableId).
        void cull(utils::FrustumPlanes const &,
                  IndexLayer indexLayer,
                  std::vector<IndexPayload *> & output) const override;

    private:
        void createImpl(IndexableId, utils::Aabb const &) override;
        void updateImpl(IndexableId, utils::Aabb const &) override;
        void eraseImpl(IndexableId) override;

    private:
        bool extend(utils::Aabb const & newAabb);
        void reshape(utils::Aabb const & newAabb);

        template<typename Callback>
        void iterateGridCells(IndexableId, Callback);

        template<typename Callback>
        void iterateTouchedCells(utils::Aabb const &, Callback);

        template<typename Callback>
        void iterateTouchedCells(utils::Aabb const &, Callback) const;

        template<typename T>
        void eraseElement(std::vector<T> & container, T element);

    private:
        using IndexableIds = std::vector<IndexableId>;

        struct GridCell
        {
            IndexableIds _items;
        };

        using GridCells = std::vector<size_t>;   // offsets in _grid

        glm::vec2 const          _tileSize;

        // forward index
        size_t                   _rows = 0;     // corresponds to x
        size_t                   _cols = 0;     // corresponds to z
        std::vector<GridCell>    _grid;         // grid of _rows * _cols items

        // backward index
        std::vector<GridCells>   _idToCells;    // offsets in _grid, maps IndexableId to a GridCells

        // a heap (primary store)
        std::vector<utils::Aabb> _idToAabb;     // AABBs of stored items

        // index parameters
        utils::Aabb              _aabb;         // the index's boundaries

        friend class SpatialHandler;
    };
}