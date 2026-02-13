#pragma once

#include <minire/utils/rect.hpp>

#include <utility>
#include <vector>

namespace minire::utils
{
    class TextLayout
    {
    public:
        explicit TextLayout(std::vector<utils::Rect> = {});

    public:
        std::optional<size_t> indexOf(float x, float y) const;
        utils::Rect const & layoutOf(size_t charIndex) const;

        bool empty() const { return _heap.empty(); }

        auto begin() { return _heap.begin(); }
        auto end() { return _heap.end(); }

        auto begin() const { return _heap.begin(); }
        auto end() const { return _heap.end(); }

        utils::Rect const & aabb() const { return _aabb; }

    private:
        void reindex();

        std::pair<size_t, size_t> pixelsToGrid(float x, float y) const;
        size_t gridIndexOf(size_t row, size_t col) const;

    private:
        using GridCell = std::vector<size_t>;

        std::vector<GridCell>    _grid;
        std::vector<utils::Rect> _heap;

        size_t                   _rows;
        size_t                   _cols;
        float                    _cellHeight;
        float                    _cellWidth;
        utils::Rect              _aabb;
    };
}