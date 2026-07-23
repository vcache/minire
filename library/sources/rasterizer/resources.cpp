#include <rasterizer/resources.hpp>

#include <minire/errors.hpp>
#include <minire/utils/overloaded.hpp>

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
            std::visit(utils::Overloaded
            {
                [this](textures::Id const & k) { _textures.erase(k); },
                [this](meshes::Id const & k) { _meshes.erase(k); },
                [this](vertex_buffers::Id const & k) { _vertexBuffers.erase(k); },
            }, it->second);
        }
        _layers.erase(layerId);
    }

    template<typename T, typename K>
    std::any const & Resources::find(T & container, K const & key) const
    {
        static std::any const kEmpty;
        auto it = container.find(key);
        return it != container.cend() ? it->second._data
                                   : kEmpty;
    }

    std::any const & Resources::find(textures::Id const & key) const
    {
        return find(_textures, key);
    }

    std::any const & Resources::find(meshes::Id const & key) const
    {
        return find(_meshes, key);
    }

    std::any const & Resources::find(vertex_buffers::Id const & key) const
    {
        return find(_vertexBuffers, key);
    }

    void Resources::insert(textures::Id key, std::any data, bool override)
    {
        insert(_textures, std::move(key), std::move(data), override);
    }

    void Resources::insert(meshes::Id key, std::any data, bool override)
    {
        insert(_meshes, std::move(key), std::move(data), override);
    }

    void Resources::insert(vertex_buffers::Id key, std::any data, bool override)
    {
        insert(_vertexBuffers, std::move(key), std::move(data), override);
    }

    template<typename T, typename K>
    void Resources::insert(T & container, K key, std::any data, bool override)
    {
        if (override)
        {
            auto existing = container.find(key);
            if (existing == container.end())
            {
                insert(container, key, data, false);
            }
            existing->second._data = std::move(data);
            // NOTE: Resource's _layer shouldn't be changed on override,
            //       event if the current layer is changed.
        }
        else
        {
            auto [_, inserted1] = container.emplace(key, Contents
                {
                    ._data = std::move(data),
                    ._layer = _current,
                });
            MINIRE_INVARIANT(inserted1, "failed to insert a new resource (a layer is \"{}\")",
                             _current); // TODO: print the Key

            _layers.emplace(_current, key);
        }
    }

    void Resources::erase(textures::Id const & key)
    {
        erase(_textures, key);
    }

    void Resources::erase(meshes::Id const & key)
    {
        erase(_meshes, key);
    }

    void Resources::erase(vertex_buffers::Id const & key)
    {
        erase(_vertexBuffers, key);
    }

    template<typename T, typename K>
    void Resources::erase(T & container, K const & key)
    {
        auto it = container.find(key);
        if (it != container.end())
        {
            Key varKey(key);
            auto cmp = [&varKey](auto const & i){ return varKey == i.second; };
            auto range = _layers.equal_range(it->second._layer);
            auto layerIt = std::find_if(range.first, range.second, cmp);
            if (layerIt != _layers.end())
            {
                _layers.erase(layerIt);
            }

            // NOTE: assuming the Key cannot be added twice at the same Layer
            assert(_layers.cend() == std::ranges::find_if(_layers, cmp));

            container.erase(it);
        }
    }
}