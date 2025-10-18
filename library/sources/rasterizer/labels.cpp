#include <rasterizer/labels.hpp>

#include <minire/errors.hpp>

#include <cassert>

namespace minire::rasterizer
{
    Labels::Labels(Fonts const & fonts)
        : _fonts(fonts)
    {}

    Label & Labels::allocate(std::string key, text::FormattedString const & text,
                             size_t z, bool visible)
    {
        auto [it, inserted] = _store.emplace(
            key, std::make_unique<Label>(_fonts, text, z, visible));
        MINIRE_INVARIANT(inserted, "label duplicate: \"{}\"", key);
        assert(it != _store.cend());
        return *it->second;
    }
    
    void Labels::deallocate(std::string const & key)
    {
        _store.erase(key);
    }

    Label & Labels::get(std::string const & key)
    {
        auto it = _store.find(key);
        MINIRE_INVARIANT(it != _store.cend(), "no such label: \"{}\"", key);
        return *it->second;
    }

    Label const & Labels::get(std::string const & key) const
    {
        auto it = _store.find(key);
        MINIRE_INVARIANT(it != _store.cend(), "no such label: \"{}\"", key);
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
