#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>

namespace minire::utils
{
    template<typename T>
    class Lerpable
    {
    public:
        explicit Lerpable(T const & init = T())
            : _current(init)
            , _items{init, init}
            , _primary(0)
            , _epochNumber(0)
        {}

        bool lerp(float weight, size_t epochNumber)
        {
            assert(epochNumber >= _epochNumber);
            if (epochNumber == _epochNumber)
            {
                _current.lerp(_items[(_primary + 1) % 2],
                              _items[_primary],
                              weight);
                return true;
            }
            else
            {
                _current = _items[_primary];
                return false;
            }

            return false;
        }

        void update(size_t epochNumber,
                    T const & newValue)
        {
            if (epochNumber == _epochNumber)
            {
                _items[_primary] = newValue;
            }
            else if (epochNumber == _epochNumber + 1)
            {
                _epochNumber = epochNumber;
                _primary = (_primary + 1) % _items.size();
                _items[_primary] = newValue;
            }
            else
            {
                _epochNumber = epochNumber;
                _items[0] = _items[1] = newValue;
            }
        }

        T const & current() const { return _current; }

        using ElementType = T;

    private:
        T                _current;
        std::array<T, 2> _items;
        size_t           _primary;
        size_t           _epochNumber; // at which it was updated
    };
}
