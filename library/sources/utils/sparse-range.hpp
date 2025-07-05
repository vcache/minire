#pragma once

#include <minire/errors.hpp>

#include <algorithm>
#include <vector>
#include <utility>

namespace minire::utils
{
    template<typename T>
    class SparseRange;

    template<typename T>
    std::string toString(SparseRange<T> const &);

    template<typename T>
    class SparseRange
    {
    public:
        // add range [begin; end]
        void insert(T begin, T end)
        {
            _items.emplace_back(0, begin);
            _items.emplace_back(1, end);
        }

    public:
        auto tighten()
        {
            std::vector<std::pair<T, T>> result;

            sort();

            T begin;
            int nesting = 0;
            for(Item const & i : _items)
            {
                switch(i.first)
                {
                    case 0: // start
                        if (0 == nesting) begin = i.second;
                        ++nesting;
                        break;

                    case 1: // end
                        --nesting;
                        if (nesting < 0)
                        {
                            MINIRE_THROW("imbalanced sparse range: {}", toString(*this));
                        }
                        if (0 == nesting)
                        {
                            result.emplace_back(begin, i.second);
                        }
                        break;

                    default: MINIRE_THROW("bad range tag: {}", int(i.first));
                }
            }

            return result;
        }

    private:
        void sort()
        {
            std::sort(_items.begin(), _items.end(),
                [](Item const & a, Item const & b)
                {
                    // NOTE: start _must_ be before end
                    if (a.second > b.second) return false;
                    if (a.second < b.second) return true;

                    return a.first < b.first;
                });
        }

    private:
        using Item = std::pair<char, T>;

        std::vector<Item> _items;

        friend std::string toString<T>(SparseRange<T> const &);
    };

    template<typename T>
    std::string toString(SparseRange<T> const & range)
    {
        std::string result;
        for(auto const & item : range._items)
        {
            result += " ";
            if (0 == item.first)
            {
                result += "[";
                result += item.second;
            }
            else
            {
                result += "]";
            }
        }
        return result;
    }
}
