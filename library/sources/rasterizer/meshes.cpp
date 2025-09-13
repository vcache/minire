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

    class MeshToken
    {
        Mesh::Uptr _mesh;

        friend class Meshes;
    };

    Meshes::Meshes(Ubo const & ubo,
                   Materials const & materials,
                   content::Manager & contentManager,
                   Resources & resources)
        : _contentManager(contentManager)
        , _resources(resources)
        , _ubo(ubo)
        , _materials(materials)
    {}

    std::shared_ptr<MeshToken> Meshes::getMesh(meshes::Id const & key)
    {
        // Look up in a resource cache
        if (std::any const & cached = _resources.find(key);
            cached.has_value())
        {
            return std::any_cast<std::shared_ptr<MeshToken>>(cached);
        }

        // Upload new mesh into a GPU and save into a cache
        auto token = load(key._contentPath, key._defaultMaterial);
        _resources.insert(key, token);

        return token;
    }

    std::shared_ptr<MeshToken> Meshes::load(content::Path const & source,
                                            material::Model::Sptr const & defaultMaterial)
    {
        std::shared_ptr<MeshToken> result = std::make_shared<MeshToken>();
        result->_mesh = std::make_unique<Mesh>(source, defaultMaterial,
                                               _contentManager, _materials, _ubo);
        return result;
    }

    void Meshes::draw(Scene const & scene) const
    {
        // TODO: group models by a material signature (to avoid frequent program switch)
        scene.cullModels(
            [&ambientLight = scene.ambientLight()]
            (MeshToken const & meshToken,
               glm::mat4 const & transform)
            {
                assert(meshToken._mesh);
                meshToken._mesh->draw(transform, ambientLight);
            }
        );
    }
}
