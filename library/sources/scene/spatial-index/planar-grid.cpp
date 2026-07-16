#include <minire/scene/spatial-index/planar-grid.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/culling-test.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>

namespace minire::scene::spatial_index
{
    namespace
    {
        bool testIntersection(float lhsMin, float lhsMax, float rhsMin, float rhsMax) {
            float const actualLhsMin = std::min(lhsMin, lhsMax);
            float const actualLhsMax = std::max(lhsMin, lhsMax);
            float const actualRhsMin = std::min(rhsMin, rhsMax);
            float const actualRhsMax = std::max(rhsMin, rhsMax);
            return actualLhsMin <= actualRhsMax && actualRhsMin <= actualLhsMax;
        }

        bool testXZIntersection(utils::Aabb const & lhs,
                                utils::Aabb const & rhs)
        {
            return testIntersection(lhs.min().x, lhs.max().x, rhs.min().x, rhs.max().x)
                && testIntersection(lhs.min().z, lhs.max().z, rhs.min().z, rhs.max().z);
        }

        template<typename Callback>
        void iterateTouchedCellsGeneric(utils::Aabb const & indexAabb,
                                        utils::Aabb const & targetAabb,
                                        glm::vec2 const & tileSize,
                                        size_t const rows,
                                        size_t const cols,
                                        Callback callback)
        {
            // early quit
            if (0 == rows || 0 == cols) return;
            if (!testXZIntersection(targetAabb, indexAabb)) return;

            // calculate an offset required to make indexAabb's "low" corner
            // to be originated in (0, 0), the same offset applies to it's
            // internal AABBs; the offset is intended to be added to coords
            glm::vec2 const offset(-indexAabb.min().x, -indexAabb.min().z);

            // shift targetAabb to ensure it in a positive quadrant of XZ
            glm::vec2 const targetMin = glm::vec2(targetAabb.min().x, targetAabb.min().z) + offset;
            glm::vec2 const targetMax = glm::vec2(targetAabb.max().x, targetAabb.max().z) + offset;
            assert(targetMin.x <= targetMax.x && targetMin.y <= targetMax.y);

            // translate world coordinates into grid indeces (inclusive indeces)
            glm::vec2 const lowerBound(.0f, .0f); // NOTE: bounds are inclusive
            glm::vec2 const upperBound(static_cast<float>(rows - 1), static_cast<float>(cols - 1));
            glm::vec2 const first = glm::clamp(glm::floor(targetMin / tileSize), lowerBound, upperBound);
            glm::vec2 const last  = glm::clamp(glm::floor(targetMax / tileSize), lowerBound, upperBound);

            size_t const firstRow = static_cast<size_t>(first.x);
            size_t const lastRow  = static_cast<size_t>(last.x);
            size_t const firstCol = static_cast<size_t>(first.y);
            size_t const lastCol  = static_cast<size_t>(last.y);

            assert(firstRow <= lastRow);
            assert(firstCol <= lastCol);
            assert(firstRow < rows && lastRow < rows);
            assert(firstCol < cols && lastCol < cols);

            // iterate over touched cells
            for(size_t row = firstRow; row <= lastRow; ++row)
            {
                for(size_t col = firstCol; col <= lastCol; ++col)
                {
                    assert(row < rows);
                    assert(col < cols);
                    size_t const cellIndex = col + row * cols;
                    callback(cellIndex);
                }
            }
        }
    }

    PlanarGrid::PlanarGrid(glm::vec2 const & tileSize)
        : _tileSize(tileSize)
    {
        MINIRE_INVARIANT(_tileSize.x > 0 && _tileSize.y > 0,
                         "tileSize must be positive, but provided: {}", _tileSize);
    }

    // If the grid is not enought to accomodate, reshape it;
    // Otherwise does nothing.
    bool PlanarGrid::extend(utils::Aabb const & newAabb)
    {
        // test if aabb's XZ is outside _aabb's XZ
        if (newAabb.min().x < _aabb.min().x || newAabb.max().x > _aabb.max().x ||
            newAabb.min().z < _aabb.min().z || newAabb.max().z > _aabb.max().z)
        {
            reshape(newAabb);
            return true;
        }
        return false;
    }

    void PlanarGrid::reshape(utils::Aabb const & newItemAabb)
    {
        // evaluate new index's AABB
        utils::Aabb aabb = newItemAabb;
        IndexableId maxIndexableId = 0;
        iterate(
            [this, &aabb, &maxIndexableId]
            (IndexableId indexableId, IndexPayload *, IndexLayer)
            {
                assert(indexableId < _idToAabb.size());
                aabb.extend(_idToAabb[indexableId]);
                maxIndexableId = std::max(maxIndexableId, indexableId);
            });

        glm::vec3 const size = aabb.max() - aabb.min();
        size_t const rows = std::max<size_t>(1, static_cast<size_t>(std::ceil(size.x / _tileSize.x)));
        size_t const cols = std::max<size_t>(1, static_cast<size_t>(std::ceil(size.z / _tileSize.y)));

        // build new storages
        std::vector<GridCell> grid(rows * cols);
        std::vector<GridCells> idToCells(maxIndexableId + 1);

        // fiil the storages with contents
        iterate(
            [this, &grid, &idToCells, &aabb, rows, cols]
            (IndexableId indexableId, IndexPayload *, IndexLayer)
            {
                assert(indexableId < _idToAabb.size());
                iterateTouchedCellsGeneric(
                    aabb, _idToAabb[indexableId], _tileSize, rows, cols,
                    [&grid, &idToCells, indexableId] (size_t const cellIndex)
                    {
                        assert(cellIndex < grid.size());
                        grid[cellIndex]._items.emplace_back(indexableId);
                        idToCells[indexableId].emplace_back(cellIndex);
                    });
            });

        // update class's state
        _rows = rows;
        _cols = cols;
        _grid = std::move(grid);
        _idToCells = std::move(idToCells);
        _aabb = aabb;
    }

    void PlanarGrid::cull(utils::FrustumPlanes const & frustumPlanes,
                          IndexLayer indexLayer,
                          std::vector<IndexPayload *> & output) const
    {
        // build AABB for the frustum (Y will is ignored)
        utils::Aabb const aabb(frustumPlanes._min, frustumPlanes._max);

        // traverse the frustum's aabb
        size_t const startOffset = output.size();
        iterateTouchedCells(aabb,
            [indexLayer, &output, this] (size_t, GridCell const & gridCell)
            {
                for(IndexableId id : gridCell._items)
                {
                    if (IndexPayload * payload = get(id);
                        layer(id) == indexLayer && payload)
                    {                        
                        output.emplace_back(payload);
                    }                   
                }
            });

        // deduplicate
        if (output.size() > startOffset)
        {
            auto begin = output.begin() + startOffset;
            std::ranges::sort(begin, output.end());
            auto ret = std::ranges::unique(begin, output.end());
            output.erase(ret.begin(), ret.end());
        }
    }

    void PlanarGrid::createImpl(IndexableId indexableId,
                                utils::Aabb const & aabb)
    {
        updateImpl(indexableId, aabb);
    }

    void PlanarGrid::updateImpl(IndexableId indexableId,
                                utils::Aabb const & aabb)
    {
        // indexableId is already allocated, whatever happens next,
        // it's AABB must stored in the heap
        _idToAabb.resize(std::max(_idToAabb.size(), indexableId + 1));
        _idToAabb[indexableId] = aabb;

        // ensure the current grid has room for the new element
        if (extend(aabb)) // may modify _idToCells, _grid, _rows, _cols, and _aabb
            return; // once reshaped, the new item would be already in the indeces

        // fetch item of backward index
        _idToCells.resize(std::max(_idToCells.size(), indexableId + 1));
        GridCells & oldGridCells = _idToCells[indexableId];

        // iterate aabb and update grid cells (or secure existing ones)
        GridCells newGridCells;
        newGridCells.reserve(oldGridCells.size());
        iterateTouchedCells(aabb,
            [this, &oldGridCells, &newGridCells, indexableId]
            (size_t index, GridCell & gridCell)
            {
#               ifndef NDEBUG
                if (oldGridCells.size() > 500)
                {
                    MINIRE_WARNING("your PlanarGrid's cells are overcrowded, consider increasing; "
                                   "_tileSize = {} oldGridCells.size() = {}", _tileSize, oldGridCells.size());
                }
#               endif

                auto it = std::ranges::find(oldGridCells, index);
                if (it != oldGridCells.end())
                {
                    // already exists, do nothing, remove from oldGridCells,
                    // to secure it from further removal,
                    // don't need to update _buckets
                    *it = oldGridCells.back();
                    oldGridCells.pop_back();
                }
                else
                {
                    // a new cell is affected, register self into this cell
                    gridCell._items.emplace_back(indexableId);
                }

                newGridCells.emplace_back(index);
            });

        // delete from old ones (non-secured residue of oldGridCells)
        for (size_t const index : oldGridCells)
        {
            assert(index < _grid.size());
            eraseElement(_grid[index]._items, indexableId);
        }

        std::swap(newGridCells, oldGridCells);
    }

    void PlanarGrid::eraseImpl(IndexableId indexableId)
    {
        // update forward index
        iterateGridCells(indexableId,
            [indexableId, this](GridCell & gridCell)
            {
                eraseElement(gridCell._items, indexableId);
            });

        // update backward index and update buckets stats
        assert(indexableId < _idToCells.size());
        _idToCells[indexableId].clear();
        _idToAabb[indexableId] = utils::Aabb();
    }

    template<typename Callback>
    void PlanarGrid::iterateGridCells(IndexableId indexableId, Callback callback)
    {
        assert(indexableId < _idToCells.size());
        for(size_t cellIndex : _idToCells[indexableId])
        {
            assert(cellIndex < _grid.size());
            callback(_grid[cellIndex]);
        }
    }

    template<typename Callback>
    void PlanarGrid::iterateTouchedCells(utils::Aabb const & aabb,
                                         Callback callback)
    {
        iterateTouchedCellsGeneric(_aabb, aabb, _tileSize, _rows, _cols,
            [this, callback](size_t const cellIndex)
            {
                assert(cellIndex < _grid.size());
                callback(cellIndex, _grid[cellIndex]);
            });
    }

    template<typename Callback>
    void PlanarGrid::iterateTouchedCells(utils::Aabb const & aabb,
                                         Callback callback) const
    {
        const_cast<PlanarGrid *>(this)->iterateTouchedCells(aabb, callback);
    }

    // assuming that elemnt MUST persists somewhere in the container
    template<typename T>
    void PlanarGrid::eraseElement(std::vector<T> & container, T element)
    {
        // find an element to delete
        assert(!container.empty());
        auto it = std::ranges::find(container, element);
        assert(it != container.end());

        // perform the removal
        *it = std::move(container.back());
        container.pop_back();
    }
}