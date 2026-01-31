#include <rasterizer/meshes.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <rasterizer/resources.hpp>
#include <scene.hpp>

#include <cassert>
#include <algorithm>

namespace minire::rasterizer
{
    // TODO: bucket drawing call to minimize program switch

    Meshes::Meshes(Ubo const & ubo,
                   Materials const & materials,
                   VertexBuffers const & vertexBuffers,
                   content::Manager & contentManager,
                   Resources & resources)
        : _contentManager(contentManager)
        , _resources(resources)
        , _ubo(ubo)
        , _materials(materials)
        , _vertexBuffers(vertexBuffers)
    {}

    std::shared_ptr<Mesh> Meshes::getMesh(meshes::Id const & key)
    {
        // Look up in a resource cache
        if (std::any const & cached = _resources.find(key);
            cached.has_value())
        {
            return std::any_cast<std::shared_ptr<Mesh>>(cached);
        }

        // Upload new mesh into a GPU and save into a cache
        auto token = load(key._contentPath, key._defaultMaterial);
        _resources.insert(key, token);

        return token;
    }

    std::shared_ptr<Mesh> Meshes::load(content::Path const & source,
                                       material::Model::Sptr const & defaultMaterial)
    {
        return std::make_shared<Mesh>(source, defaultMaterial,
                                      _contentManager, _materials, _ubo,
                                      _vertexBuffers);
    }

    void Meshes::draw(Scene const & scene,
                      material::TextureRefs const & directionalLightsShadowMaps,
                      material::TextureRefs const & pointLightsShadowMaps) const
    {
        // TODO: group models by a material signature (to avoid frequent program switch)
        scene.cullModels(
            [&ambientLight = scene.ambientLight(),
             &directionalLightsShadowMaps, &pointLightsShadowMaps]
            (Mesh const & mesh, glm::vec3 const & emissiveFactor,
             glm::mat4 const & transform, material::SkinningVector const & skinningVector)
            {
                mesh.draw(transform, ambientLight, emissiveFactor,
                          directionalLightsShadowMaps,
                          pointLightsShadowMaps,
                          skinningVector);
            }
        );
    }
}
