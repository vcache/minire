#pragma once

#include <minire/events/controller/scene.hpp>

#include <scene/model.hpp>
#include <scene/point-light.hpp>

#include <array>
#include <vector>
#include <memory>
#include <unordered_set>

namespace minire::utils { class Viewpoint; }

namespace minire
{
    class Rasterizer;

    class Scene
    {
    public:
        explicit Scene(Rasterizer &);

        ~Scene();

    public:
        void handle(events::controller::SceneReset const &);

        void handle(events::controller::SceneEmergeModel const &);

        void handle(events::controller::SceneEmergePointLight const &);

        void handle(events::controller::SceneUnmergeModel const &);

        void handle(events::controller::SceneUnmergePointLight const &);

        void handle(size_t epochNumber,
                    events::controller::SceneUpdateModel const & e);

        void handle(events::controller::SceneSetSelectedModels const & e);
        
        void handle(size_t epochNumber,
                    events::controller::SceneUpdateLight const &);

    public:
        void lerp(float weight, size_t epochNumber);

        scene::ModelRef::List cullModels(utils::Viewpoint const &) const;

        scene::PointLightRef::List cullPointLights(utils::Viewpoint const &, size_t) const;

    private:
        scene::Model::Uptr & getModel(size_t id);
        scene::PointLight::Uptr & getPointLight(size_t id);

        void lerpModels(float weight, size_t epochNumber);
        void lerpLights(float weight, size_t epochNumber);

    private:
        Rasterizer                         & _rasterizer;

        // models
        std::vector<scene::Model::Uptr>      _models;   // TODO: why not unordered_map?
        std::unordered_set<size_t>           _activeModels;

        // lights
        std::vector<scene::PointLight::Uptr> _lights;   // TODO: why not unordered_map?
        std::unordered_set<size_t>           _activeLights;

        // miscellaneous
        std::unordered_set<size_t>           _selectedModelsIds;
    };
}
