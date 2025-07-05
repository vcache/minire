#include <scene.hpp>

#include <gpu/render.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/geometry.hpp>
#include <utils/viewpoint.hpp>

#include <cassert>

// TODO: merge/unmerge will leak on exceptions

namespace minire
{
    Scene::Scene(gpu::Render & gpuRender)
        : _gpuRender(gpuRender)
    {}

    Scene::~Scene()
    {
        handle(events::controller::SceneReset());
    }

    void Scene::handle(events::controller::SceneReset const &)
    {
        MINIRE_DEBUG("handling SceneReset");

        for(scene::Model::Uptr const & model : _models)
        {
            if (model) _gpuRender.models().decUse(model->model());
        }
        _activeModels.clear();
        _models.clear();

        _activeLights.clear();
        _lights.clear();
    }
    
    void Scene::handle(events::controller::SceneEmergeModel const & e)
    {
        if (_models.size() <= e._id) _models.resize(e._id + 1);
        if (_models[e._id]) MINIRE_THROW("model slot busy: {}", e._id);

        _gpuRender.models().incUse(e._model);
        auto model = std::make_unique<scene::Model>(
            e._id,
            e._model,
            _gpuRender.models().aabb(e._model),
            e._position);
        _models[e._id] = std::move(model);
    }
    
    void Scene::handle(events::controller::SceneEmergePointLight const & e)
    {
        if (_lights.size() <= e._id) _lights.resize(e._id + 1);
        if (_lights[e._id]) MINIRE_THROW("light slot busy: {}", e._id);
        auto light = std::make_unique<scene::PointLight>(e._light);
        _lights[e._id] = std::move(light);
    }
    
    void Scene::handle(events::controller::SceneUnmergeModel const & e)
    {
        auto & model = getModel(e._id);
        _gpuRender.models().decUse(model->model());
        model.reset();
        _activeModels.erase(e._id);

        // TODO: maybe shrink _models or calc upper bound of _models
        //       to avoid iterationing over the tail of empty slots
    }
    
    void Scene::handle(events::controller::SceneUnmergePointLight const & e)
    {
        auto & light = getPointLight(e._id);
        light.reset();
        _activeLights.erase(e._id);

        // TODO: maybe shrink _lights or calc upper bound of _models
        //       to avoid iterationing over the tail of empty slots
    }
    
    void Scene::handle(size_t epochNumber,
                       events::controller::SceneUpdateModel const & e)
    {
        getModel(e._id)->update(epochNumber, e._position);
        _activeModels.insert(e._id);
    }

    void Scene::handle(events::controller::SceneSetSelectedModels const & e)
    {
        _selectedModelsIds = e._ids;
    }
    
    void Scene::handle(size_t epochNumber,
                       events::controller::SceneUpdateLight const & e)
    {
        getPointLight(e._id)->update(epochNumber, e._light);
        _activeLights.insert(e._id);
    }

    scene::Model::Uptr & Scene::getModel(size_t id)
    {
        if (id >= _models.size())
        {
            MINIRE_THROW("no such model on scene, id={}, size={}",
                         id, _models.size());
        }
        if (!_models[id]) MINIRE_THROW("model not created, id={}", id);
        return _models[id];
    }

    scene::PointLight::Uptr & Scene::getPointLight(size_t id)
    {
        if (id >= _lights.size())
        {
            MINIRE_THROW("no such light on scene, id={}, size={}",
                         id, _lights.size());
        }
        if (!_lights[id]) MINIRE_THROW("light not created, id={}", id);
        return _lights[id];
    }

    void Scene::lerp(float weight, size_t epochNumber)
    {
        lerpModels(weight, epochNumber);
        lerpLights(weight, epochNumber);
        // TODO: lerp camera
    }

    void Scene::lerpModels(float weight, size_t epochNumber)
    {
        auto it = _activeModels.begin(),
            end = _activeModels.end();
        while (it != end)
        {
            scene::Model::Uptr const & model = _models[*it];
            if (model)
            {
                bool stillActive = model->lerp(weight, epochNumber);
                if (!stillActive)
                {
                    it = _activeModels.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }
    }

    void Scene::lerpLights(float weight, size_t epochNumber)
    {
        auto it = _activeLights.begin(),
            end = _activeLights.end();
        while (it != end)
        {
            scene::PointLight::Uptr const & light = _lights[*it];
            if (light)
            {
                bool stillActive = light->lerp(weight, epochNumber);
                if (!stillActive)
                {
                    it = _activeLights.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }
    }

    scene::ModelRef::List
    Scene::cullModels(utils::Viewpoint const &) const
    {
        // prepare resulting models set
        scene::ModelRef::List result;
        for(scene::Model::Uptr const & model : _models)
        {
            assert(model);
            bool const selected = 0 != _selectedModelsIds.count(model->id());
            result.emplace_back(model->model(),
                                model->transform(),
                                selected ? 1.5f : 1.0f);
        }

        // TODO: sort by "front-to-back"

        return result;
    }

    scene::PointLightRef::List Scene::cullPointLights(utils::Viewpoint const &,
                                                      size_t maxLights) const
    {
        scene::PointLightRef::List result;

        result.reserve(maxLights);
        for(scene::PointLight::Uptr const & light: _lights)
        {
            if (!light) continue;
            if (result.size() >= maxLights) break;
            result.emplace_back(light->current());
        }

        // TODO: sort by "front-to-back"

        // TODO: sort by distance and cull the farest

        return result;
    }

}
