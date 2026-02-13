#include <minire/utils/glyph-grid.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>

namespace minire::utils
{
    TextLayout::TextLayout(std::vector<Rect> characters)
        : _grid()
        , _heap(std::move(characters))
        , _rows(0)
        , _cols(0)
        , _cellHeight(0)
        , _cellWidth(0)
    {
        reindex();
    }

    std::optional<size_t> TextLayout::indexOf(float x, float y) const
    {
        if (empty()) return std::nullopt;

        auto const [row, col] = pixelsToGrid(x, y);
        size_t const gridIndex = gridIndexOf(row, col);
        assert(gridIndex < _grid.size());
        GridCell const & gridCell = _grid[gridIndex];
        for(size_t i : gridCell)
        {
            assert(i < _heap.size());
            utils::Rect const & rect = _heap[i];

            if (rect._left <= x && x <= rect._right &&
                rect._top <= y && y <= rect._bottom)
            {
                return i;
            }
        }
        return std::nullopt;
    }

    utils::Rect const & TextLayout::layoutOf(size_t charIndex) const
    {
        assert(charIndex < _heap.size());
        return _heap.at(charIndex);
    }

    std::pair<size_t, size_t> TextLayout::pixelsToGrid(float x, float y) const
    {
        assert(_cellHeight > 0.0f);
        assert(_cellWidth > 0.0f);

        assert(_rows >= 1);
        assert(_cols >= 1);

        size_t const row = std::min<size_t>(_rows - 1, std::floor(std::max(0.0f, y) / _cellHeight));
        size_t const col = std::min<size_t>(_cols - 1, std::floor(std::max(0.0f, x) / _cellWidth));
        return std::make_pair(row, col);
    }

    size_t TextLayout::gridIndexOf(size_t row, size_t col) const
    {
        assert(row < _rows);
        assert(col < _cols);
        return col + row * _cols;
    }

    void TextLayout::reindex()
    {
        // intialization
        _grid.clear();
        _rows = 0;
        _cols = 0;
        _cellHeight = 0.0f;
        _cellWidth = 0.0f;
        _aabb = utils::Rect();

        if (_heap.empty())
            return;

        // calculate the layout's boundaries
        _aabb = _heap.front();
        _cellWidth = _aabb._right - _aabb._left;
        _cellHeight = _aabb._bottom - _aabb._top;
        for(utils::Rect const & i : _heap)
        {
            assert(i._left >= 0);
            assert(i._top >= 0);
            assert(i._right >= 0);
            assert(i._bottom >= 0);

            assert(i._left <= i._right);
            assert(i._top <= i._bottom);

            _aabb._left = std::min(_aabb._left, i._left);
            _aabb._top = std::min(_aabb._top, i._top);
            _aabb._right = std::max(_aabb._right, i._right);
            _aabb._bottom = std::max(_aabb._bottom, i._bottom);

            _cellWidth = std::max(_cellWidth, _aabb._right - _aabb._left);
            _cellHeight = std::max(_cellHeight, _aabb._bottom - _aabb._top);
        }

        assert(_aabb._left <= _aabb._right);
        assert(_aabb._top <= _aabb._bottom);

        // eval grid size
        _rows = std::abs(_aabb._bottom - _aabb._top) / _cellHeight;
        _rows = std::max<size_t>(_rows, 1);

        _cols = std::abs(_aabb._right - _aabb._left) / _cellWidth;
        _cols = std::max<size_t>(_cols, 1);

        // fill the grid
        assert(_rows >= 1);
        assert(_cols >= 1);
        _grid.resize(_rows * _cols, GridCell{});
        for(size_t i = 0; i < _heap.size(); ++i)
        {
            Rect const & rect = _heap[i];
            auto lt = pixelsToGrid(rect._left, rect._top);
            auto br = pixelsToGrid(rect._right, rect._bottom);

            auto [rowFirst, rowLast] = std::minmax(lt.first, br.first);
            auto [colFirst, colLast] = std::minmax(lt.second, br.second);

            for(size_t row = rowFirst; row <= std::min(rowLast, _rows - 1); ++row)
            {
                for(size_t col = colFirst; col <= std::min(colLast, _cols - 1); ++col)
                {
                    size_t gridIndex = gridIndexOf(row, col);
                    assert(gridIndex < _grid.size());
                    _grid[gridIndex].push_back(i);
                }
            }
        }
    }
}