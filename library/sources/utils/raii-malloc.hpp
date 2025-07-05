#pragma once

#include <cassert>
#include <cstddef> // for size_t
#include <utility> // for std::swap

namespace minire::utils
{
    template<class T>
    class RaiiMalloc
    {
        RaiiMalloc(RaiiMalloc const &) = delete;
        RaiiMalloc& operator=(RaiiMalloc const &) = delete;

    public:
        explicit RaiiMalloc(size_t size)
            : _data(new T[size])
            , _size(size)
        {}

        ~RaiiMalloc() { delete []_data; }

        RaiiMalloc(RaiiMalloc && o)
            : _data(o._data)
            , _size(o._size)
        {
            o._data = nullptr;
            o._size = 0;
        }

        RaiiMalloc& operator=(RaiiMalloc && o)
        {
            RaiiMalloc tmp(std::move(o));
            std::swap(_data, tmp._data);
            std::swap(_size, tmp._size);
            return *this;
        }

    public:
        size_t size() const { return _size; }

        T * data() const { return _data; }

    public:
        T const & operator[](size_t i) const { assert(i < _size);  return _data[i]; }

        T & operator[](size_t i) { assert(i < _size); return _data[i]; }

    public:
        size_t bytesCount() const
        {
            return _size * sizeof(T);
        }

        void const * bytesPointer() const
        {
            return reinterpret_cast<void const*>(_data);
        }

    private:
        T *    _data;
        size_t _size;
    };
}
