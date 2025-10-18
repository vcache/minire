#include <minire/gui/layouts/grid.hpp>

#include <minire/errors.hpp>
#include <minire/gui/component.hpp>
#include <minire/logging.hpp>

#include <cassert>
#include <cmath>

namespace minire::gui::layouts
{
    Grid::Grid(size_t rows, size_t cols)
        : _rows(rows)
        , _cols(cols)
        , _cells(rows * cols)
    {
        for(size_t row = 0; row < _rows; ++row)
        {
            for(size_t col = 0; col < _cols; ++col)
            {
                Cell & cell = _cells[indexOf(row, col)];
                cell._left = static_cast<float>(col);
                cell._top = static_cast<float>(row);
            }
        }
    }

    std::optional<std::string> const & Grid::get(size_t row, size_t col) const
    {
        return _cells[indexOf(row, col)]._id;
    }

    void Grid::set(size_t row, size_t col, std::string id)
    {
        // ensure that destination isn't busy
        size_t const index = indexOf(row, col);
        Cell & cell = _cells[index];
        MINIRE_INVARIANT(!cell._id.has_value(),
                         "cell {}x{} is already used by {}",
                         row, col, *(cell._id));

        // maybe unset "id" from current location
        unset(id);

        // put "id" into a new cell
        cell._id = id;
        auto [_, inserted] = _mapping.emplace(id, index);
        MINIRE_INVARIANT(inserted, "failed to insert {} into a grid", id);
    }

    void Grid::unset(size_t row, size_t col)
    {
        if (Cell const & cell = _cells[indexOf(row, col)];
            cell._id.has_value())
        {
            unset(*cell._id);
        }
    }

    void Grid::unset(std::string const & id)
    {
        unsetImpl(id);
    }

    bool Grid::unsetImpl(std::string const & id)
    {
        if (auto it = _mapping.find(id);
            it != _mapping.cend())
        {
            assert(it->second < _cells.size());
            Cell & cell = _cells[it->second];
            assert(cell._id.has_value());
            assert(*cell._id == id);
            assert(it->second < _cells.size());

            cell._id.reset();
            _mapping.erase(it);

            return true;
        }

        return false;
    }

    Area Grid::evaluate(Area const & client,
                        Component const & component) const
    {
        if (auto it = _mapping.find(component.id());
            it != _mapping.cend())
        {
            assert(it->second < _cells.size());
            Cell const & cell = _cells[it->second];

            float const cellWidth = std::floor(client._width / static_cast<float>(_cols));
            float const cellHeight = std::floor(client._height / static_cast<float>(_rows));

            return Area
            {
                ._left = client._left + cellWidth  * cell._left,
                ._top = client._top   + cellHeight * cell._top,
                ._width = cellWidth,
                ._height = cellHeight,
            };
        }
        else
        {
            MINIRE_WARNING("cannot perform grid-layout for {}, since it is not set",
                           component.id());
            return client;
        }
    }

    void Grid::onErase(Component const & component)
    {
        unsetImpl(component.id());
    }

    void Grid::onClear()
    {
        _mapping.clear();
        for(Cell & cell : _cells)
        {
            cell._id.reset();
        }
    }

    size_t Grid::indexOf(size_t row, size_t col) const
    {
        MINIRE_INVARIANT(row < _rows && col < _cols,
                         "grid position is out of borders: {}x{}, while size is {}x{}",
                         row, col, _rows, _cols);
        return col + row * _cols;
    }
}
