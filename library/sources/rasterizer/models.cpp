#include <rasterizer/models.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <cassert>
#include <algorithm>

namespace minire::rasterizer
{
    // TODO: bucket drawing call to minimize program switch

    Models::Models(Ubo const & ubo,
                   Textures const & textures,
                   content::Manager & contentManager)
        : _contentManager(contentManager)
        , _ubo(ubo)
        , _textures(textures)
    {}

    void Models::incUse(content::Id const & id)
    {
        load(id);
        ++_store[id]._usage;
    }
    
    void Models::decUse(content::Id const & id)
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

    void Models::load(content::Id const & id)
    {
        // find store item
        auto & item = _store[id];

        if (!item._init)
        {
            // load a model itself
            auto lease = _contentManager.borrow(id);
            assert(lease);
            models::SceneModel const & sceneModel = lease->as<models::SceneModel>();
            item._model = std::make_unique<Model>(sceneModel, _contentManager,
                                                  _textures, _ubo);
            item._programKey = item._model->flags();

            // ensure there is a program for a model
            if (_programs.cend() == _programs.find(item._programKey))
            {
                _programs.emplace(item._programKey,
                                  item._model->makeProgram());
                MINIRE_INFO("Loading model program: {}", item._programKey);
            }

            // mark slot as initialized
            item._init = true;
            MINIRE_INFO("Loading model: {}", id);
        }
    }

    void Models::unload(content::Id const & id)
    {
        if (_store[id]._init)
        {
            MINIRE_ERROR("unloading not implemented");
        }
    }

    utils::Aabb const & Models::aabb(content::Id const & id) const
    {
        auto const & it = _store.find(id);
        if (it == _store.cend() || !it->second._init)
        {
            MINIRE_THROW("model {} is not loaded", id);
        }
        assert(it->second._model);
        return it->second._model->aabb();
    }

    void Models::draw(scene::ModelRef::List & entities) const
    {
        // sort entities by render program (to avoid frequent program switch)
        // TODO TODO TODO: avoid sorting, just store a Model inside Program's block
        std::sort(entities.begin(), entities.end(),
            [this](scene::ModelRef const & a,
                   scene::ModelRef const & b)
            {
                auto const & ai = _store.at(a._model);
                auto const & bi = _store.at(b._model);

                assert(ai._init);
                assert(bi._init);

                return ai._programKey < bi._programKey;
            });

        // render entities
        ProgramKey current = kEmptyProgramKey;
        Model::Program::Sptr currentPtr;
        for(scene::ModelRef const & entity : entities)
        {
            StoreItem const & modelData = _store.at(entity._model);

            assert(modelData._model);
            assert(modelData._programKey != kEmptyProgramKey);
            assert(modelData._init);

            if (current != modelData._programKey)
            {
                current = modelData._programKey;
                currentPtr = _programs.at(current);
                assert(currentPtr);
                
                currentPtr->use();
            }

            assert(entity._transform);
            modelData._model->draw(*currentPtr, *entity._transform, entity._colorFactor);
        }
    }
}
