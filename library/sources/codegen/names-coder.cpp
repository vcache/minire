#include <codegen/names-coder.hpp>

#include <minire/errors.hpp>

#include <algorithm>

namespace minire::codegen
{
    size_t NamesCoder::getOrMakeUniformCode(std::string const & name)
    {
        if (auto it = std::ranges::find(_store, name);
            it != _store.cend())
        {
            return std::distance(_store.begin(), it);
        }
        _store.push_back(name);
        return _store.size() - 1;
    }

    size_t NamesCoder::find(std::string const & name) const
    {
        auto it = std::ranges::find(_store, name);
        MINIRE_INVARIANT(it != _store.cend(), "no such name: {}", name);
        return std::distance(_store.cbegin(), it);
    }
}
