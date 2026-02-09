#pragma once

#include <minire/gui/layout.hpp>

#include <optional>
#include <unordered_map>
#include <vector>

namespace minire::gui::layouts
{
    class Grid
        : public LinearLayout
    {
    public:
        Grid(size_t rows, size_t cols);

        Area evaluate(Area const & client,
                      Component const &) const override;

        void onErase(Component const &) override;

        void onClear() override;

        std::optional<std::string> const & get(size_t row, size_t col) const;

        void set(size_t row, size_t col, std::string);

        void unset(size_t row, size_t col);

        void unset(std::string const &);

        size_t rows() const { return _rows; }

        size_t cols() const { return _cols; }

    private:
        size_t indexOf(size_t row, size_t col) const;

        bool unsetImpl(std::string const &);

    private:
        struct Cell
        {
            std::optional<std::string> _id;
            float                      _left = 0;
            float                      _top = 0;
        };

        using Cells = std::vector<Cell>;
        using Mapping = std::unordered_map<std::string, size_t>;

        size_t const _rows;
        size_t const _cols;
        Cells        _cells;
        Mapping      _mapping;
    };
}
