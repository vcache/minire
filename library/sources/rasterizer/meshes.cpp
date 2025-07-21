#include <rasterizer/meshes.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <cassert>
#include <algorithm>

namespace minire::rasterizer
{
    // TODO: bucket drawing call to minimize program switch

    Meshes::Meshes(Ubo const & ubo,
                   Materials const & materials,
                   content::Manager & contentManager)
        : _contentManager(contentManager)
        , _ubo(ubo)
        , _materials(materials)
    {}

    void Meshes::incUse(content::Id const & id)
    {
        load(id);
        ++_store[id]._usage;
    }
    
    void Meshes::decUse(content::Id const & id)
    {
        if (!_store[id]._init)
        {
            MINIRE_THROW("cannot decrease usage for non-loaded model: {}", id);
        }

        --_store[id]._usage;
        assert(_store[id]._usage >= 0);
        if (0 == _store[id]._usage)
        {
            unload(id);
        }
    }

    void Meshes::load(content::Id const & id)
    {
        // find store item
        auto & item = _store[id];

        if (!item._init)
        {
            // load a model itself
            auto lease = _contentManager.borrow(id);
            assert(lease);
            models::SceneModel const & sceneModel = lease->as<models::SceneModel>();
            item._model = std::make_unique<Mesh>(id, sceneModel, _contentManager,
                                                 _materials, _ubo);

            // mark slot as initialized
            item._init = true;
            MINIRE_INFO("Loading model: {}", id);
        }
    }

    void Meshes::unload(content::Id const & id)
    {
        if (_store[id]._init)
        {
            MINIRE_ERROR("unloading not implemented");
        }
    }

    utils::Aabb const & Meshes::aabb(content::Id const & id) const
    {
        auto const & it = _store.find(id);
        if (it == _store.cend() || !it->second._init)
        {
            MINIRE_THROW("model {} is not loaded", id);
        }
        assert(it->second._model);
        return it->second._model->aabb();
    }

    void Meshes::draw(scene::ModelRef::List & entities) const
    {
        // TODO: group models by a material signature (to avoid frequent program switch)

        for(scene::ModelRef const & entity : entities)
        {
            StoreItem const & modelData = _store.at(entity._model);

            assert(modelData._model);
            assert(modelData._init);
            assert(entity._transform);

            modelData._model->draw(*entity._transform, entity._colorFactor);
        }
    }
}
