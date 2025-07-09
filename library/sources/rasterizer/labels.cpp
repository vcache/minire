#include <rasterizer/labels.hpp>

#include <minire/errors.hpp>

#include <cassert>

namespace minire::rasterizer
{
    Labels::Labels(Fonts const & fonts)
        : _fonts(fonts)
    {}

    Label & Labels::allocate(std::string key, int z, bool visible)
    {
        auto res = _store.emplace(std::move(key),
                                  std::make_unique<Label>(_fonts, z, visible));
        if (!res.second)
        {
            MINIRE_THROW("label duplicate: \"{}\"", key);
        }
        return *(res.first->second);
    }
    
    void Labels::deallocate(std::string const & key)
    {
        _store.erase(key);
    }

    Label & Labels::get(std::string const & key)
    {
        auto it = _store.find(key);
        if (it == _store.cend())
        {
            MINIRE_THROW("no such label: \"{}\"", key);
        }
        return *it->second;
    }

    Label const & Labels::get(std::string const & key) const
    {
        auto it = _store.find(key);
        if (it == _store.cend())
        {
            MINIRE_THROW("no such label: \"{}\"", key);
        }
        return *it->second;
    }

    void Labels::predraw(Drawable::PtrsList & out) const
    {
        for(auto const & label : _store)
        {
            assert(label.second);
            if (label.second->visible())
            {
                out.push_back(label.second.get());
            }
        }
    }
}