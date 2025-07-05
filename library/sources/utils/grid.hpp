#pragma once

#include <cassert>
#include <cstdint>
#include <vector>

#include <minire/errors.hpp>

namespace minire::utils
{
    // TODO: cover with tests !!

    template<typename T>
    class Grid
    {
    public:
        Grid(): _rows(0), _cols(0) {}

        Grid(Grid &&) = default;

        Grid(Grid const &) = default;

        Grid & operator=(Grid &&) = default;

        Grid & operator=(Grid const &) = default;

        explicit Grid(size_t rows, size_t cols, T const & init = T())
            : _store(rows * cols, init)
            , _rows(rows)
            , _cols(cols)
        {}

        void resize(size_t rows, size_t cols, T const & init = T())
        {
            Store n(rows * cols, init);

            for(size_t r(0); r < std::min(rows, _rows); ++r)
            {
                for(size_t c(0); c < std::min(cols, _cols); ++c)
                {
                    n[c + r * cols] = _store[index(r, c)];
                }
            }

            _store.swap(n);
            _rows = rows;
            _cols = cols;
        }

        void clear()
        {
            _store.clear();
            _rows = _cols = 0;
        }

    public:
        T const & at(size_t row, size_t column) const
        {
            return _store[index(row, column)];
        }
        
        T & at(size_t row, size_t column)
        {
            return _store[index(row, column)];
        }

        T const & at(size_t i) const
        {
            assert(i < _store.size());
            return _store[i];
        }

        T & at(size_t i)
        {
            assert(i < _store.size());
            return _store[i];
        }

    public:
        size_t rows() const { return _rows; }

        size_t cols() const { return _cols; }

        size_t size() const { return _store.size(); }

        auto begin() const { return _store.begin(); }
        
        auto end() const { return _store.end(); }

    private:
        size_t index(size_t row, size_t col) const
        {
            MINIRE_INVARIANT(row < _rows, "rows overflow: row = {}, rows = {}", row, _rows);
            MINIRE_INVARIANT(col < _cols, "cols overflow: col = {}, cols = {}", col, _cols);
            return col + row * _cols;
        }

    private:
        using Store = std::vector<T>;

        Store  _store;
        size_t _rows = 0;
        size_t _cols = 0;
    };
}
