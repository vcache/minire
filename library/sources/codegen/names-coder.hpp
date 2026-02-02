#pragma once

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace minire::codegen
{
    // Maps names into persistent uniform codes which in turn will be mapped to
    // program's uniform ids.
    class NamesCoder
    {
    public:
        size_t getOrMakeUniformCode(std::string const & name);

        size_t size() const { return _store.size(); }

        std::string const & operator[](size_t i) const
        {
            assert(i < _store.size());
            return _store[i];
        }

        size_t find(std::string const & name) const;

    private:
        std::vector<std::string> _store;
    };
}