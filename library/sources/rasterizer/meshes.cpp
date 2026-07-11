#include <rasterizer/meshes.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <rasterizer/resources.hpp>
#include <scene-impl.hpp>

#include <cassert>
#include <algorithm>

namespace minire::rasterizer
{
    Meshes::Meshes(Materials const & materials,
                   VertexBuffers const & vertexBuffers,
                   content::Manager & contentManager,
                   Resources & resources)
        : _contentManager(contentManager)
        , _resources(resources)
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
                                       Material::Sptr const & defaultMaterial)
    {
        return std::make_shared<Mesh>(source, defaultMaterial, _contentManager, _materials,
                                      _vertexBuffers);
    }

    void Meshes::draw(SceneImpl const & scene,
                      CulledPrimitives const & culledPrimitives,
                      material::TextureRefs const & directionalLightsShadowMaps,
                      material::TextureRefs const & pointLightsShadowMaps) const
    {
        // TODO: group models by brush signatures (to avoid frequent program switch)
        glm::vec3 const ambientLight = scene.ambientLight();
        for(auto const & [uniquePrimitive, primitiveInstances] : culledPrimitives)
        {
            Materials::Brush const & brush =
                uniquePrimitive._mesh.brush(uniquePrimitive._primitiveIndex);
            brush.draw(uniquePrimitive, primitiveInstances, ambientLight,
                       directionalLightsShadowMaps, pointLightsShadowMaps);

        }
    }
}
