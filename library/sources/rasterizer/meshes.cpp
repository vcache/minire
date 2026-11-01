#include <rasterizer/meshes.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <rasterizer/draw-batcher.hpp>
#include <rasterizer/resources.hpp>
#include <scene-impl.hpp>

#include <cassert>
#include <algorithm>

namespace minire::rasterizer
{
    Meshes::Meshes(Materials const & materials,
                   VertexBuffers const & vertexBuffers,
                   SharedVertexBuffers & sharedVertexBuffers,
                   content::Manager & contentManager,
                   Resources & resources)
        : _contentManager(contentManager)
        , _resources(resources)
        , _materials(materials)
        , _vertexBuffers(vertexBuffers)
        , _sharedVertexBuffers(sharedVertexBuffers)
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

    std::shared_ptr<Mesh> Meshes::load(content::Handle const & source,
                                       Material::Sptr const & defaultMaterial)
    {
        return std::make_shared<Mesh>(source, defaultMaterial, _contentManager, _materials,
                                      _vertexBuffers, _sharedVertexBuffers);
    }

    void Meshes::draw(SceneImpl const & scene,
                      CulledPrimitives const & culledPrimitives,
                      material::TextureRefs const & directionalLightsShadowMaps,
                      material::TextureRefs const & pointLightsShadowMaps) const
    {
        glm::vec3 const ambientLight = scene.ambientLight();

        DrawBatcher batcher;
        batcher.collect(culledPrimitives, [](UniquePrimitive const & prim) -> Materials::Brush const & {
            return prim._mesh.brush(prim._primitiveIndex);
        });
        batcher.draw(ambientLight, directionalLightsShadowMaps, pointLightsShadowMaps);
    }
}
