#pragma once

#include <rasterizer/meshes/id.hpp>
#include <rasterizer/textures/id.hpp>

#include <any>
#include <string>
#include <unordered_map>
#include <variant>

namespace minire::rasterizer
{
    /**
     * Resource - anything uploaded into a GPU (texture, VBO, VAO, etc).
     *            Resource are immutable.
     * Layer - a set of grouped resources.
     * Current Layer - the layer to which new resources are added (it always exists).
     * Key - an identifier of a Resource, globally unique (i.e. a Layer is NOT an isolation unit).
     * */
    class Resources
    {
    public:
        // NOTE: LayerId can be "". Although seems strange, but must be a valid case.
        using LayerId = std::string;
        using Key = std::variant<textures::Id, meshes::Id>;

    public:
        // creates a new Layer and makes it current;
        // if the layer already exists, just make it current
        void newLayer(LayerId const &);

        // immediately destroy layer and its resources;
        // if provided Layer is the current, resources will be disposed,
        // but the Layer continue to exists (i.e. any new inserts will be
        // done into this Layer)
        void disposeLayer(LayerId const &);

        LayerId const & current() const { return _current; }

    public:
        std::any const & find(Key const &) const;

        void insert(Key, std::any);

        // TODO: this one might have linear complexity
        void erase(Key const &);

    private:
        struct Contents
        {
            std::any _data;
            LayerId  _layer;
        };

        LayerId                               _current;
        std::unordered_map<Key, Contents>     _store;
        std::unordered_multimap<LayerId, Key> _layers;
    };
}