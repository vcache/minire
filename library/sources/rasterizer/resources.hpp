#pragma once

#include <rasterizer/meshes/id.hpp>
#include <rasterizer/textures/id.hpp>
#include <rasterizer/vertex-buffers/id.hpp>

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
        std::any const & find(textures::Id const &) const;
        std::any const & find(meshes::Id const &) const;
        std::any const & find(vertex_buffers::Id const &) const;

        void insert(textures::Id, std::any, bool override = false);
        void insert(meshes::Id, std::any, bool override = false);
        void insert(vertex_buffers::Id, std::any, bool override = false);

        // TODO: this one might have linear complexity
        void erase(textures::Id const &);
        void erase(meshes::Id const &);
        void erase(vertex_buffers::Id const &);

    private:
        template<typename T, typename K>
        std::any const & find(T & container, K const & key) const;

        template<typename T, typename K>
        void insert(T & container, K key, std::any data, bool override);

        template<typename T, typename K>
        void erase(T & container, K const & key);

    private:
        using Key = std::variant<textures::Id,
                                 meshes::Id,
                                 vertex_buffers::Id>;

        struct Contents
        {
            std::any _data;
            LayerId  _layer;
        };

        LayerId                                          _current;
        std::unordered_map<textures::Id, Contents>       _textures;
        std::unordered_map<meshes::Id, Contents>         _meshes;
        std::unordered_map<vertex_buffers::Id, Contents> _vertexBuffers;
        std::unordered_multimap<LayerId, Key>            _layers;
    };
}