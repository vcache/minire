#include <rasterizer/resources.hpp>

#include <minire/errors.hpp>

#include <algorithm>
#include <cassert>

namespace minire::rasterizer
{
    void Resources::newLayer(LayerId const & layerId)
    {
        _current = layerId;
    }

    void Resources::disposeLayer(LayerId const & layerId)
    {
        auto range = _layers.equal_range(layerId);
        for (auto it = range.first; it != range.second; ++it)
        {
            Key const & key = it->second;
            _store.erase(key);
        }
        _layers.erase(layerId);
    }

    std::any const & Resources::find(Key const & key) const
    {
        static std::any const kEmpty;
        auto it = _store.find(key);
        return it != _store.cend() ? it->second._data
                                   : kEmpty;
    }

    void Resources::insert(Key key, std::any data, bool override)
    {
        if (override)
        {
            auto existing = _store.find(key);
            if (existing == _store.end())
            {
                insert(key, data, false);
            }
            existing->second._data = std::move(data);
            // NOTE: Resource's _layer shouldn't be changed on override,
            //       event if the current layer is changed.
        }
        else
        {
            auto [_, inserted1] = _store.emplace(key, Contents
                {
                    ._data = std::move(data),
                    ._layer = _current,
                });
            MINIRE_INVARIANT(inserted1, "failed to insert a new resource (a layer is \"{}\")",
                             _current); // TODO: print the Key

            _layers.emplace(_current, key);
        }
    }

    void Resources::erase(Key const & key)
    {
        auto it = _store.find(key);
        if (it != _store.end())
        {
            auto cmp = [&key](auto const & i){ return key == i.second; };
            auto range = _layers.equal_range(it->second._layer);
            auto layerIt = std::find_if(range.first, range.second, cmp);
            if (layerIt != _layers.end())
            {
                _layers.erase(layerIt);
            }

            // NOTE: assuming the Key cannot be added twice at the same Layer
            assert(_layers.cend() == std::ranges::find_if(_layers, cmp));

            _store.erase(it);
        }
    }
}