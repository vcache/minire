#include <rasterizer/meshes.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>

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
                   content::Manager & contentManager)
        : _contentManager(contentManager)
        , _ubo(ubo)
        , _materials(materials)
    {}

    std::shared_ptr<MeshToken> Meshes::getMesh(content::Path const & source,
                                               material::Model::Sptr const & defaultMaterial)
    {
        Key key(source, defaultMaterial);

        if (auto it = _store.find(key);
            it != _store.cend())
        {
            if (auto token = it->second.lock(); token)
            {
                return token;
            }
            else
            {
                _store.erase(it);
            }
        }

        auto token = load(source, defaultMaterial);
        auto [_, inserted] = _store.emplace(key, token);
        MINIRE_INVARIANT(inserted, "failed to store MeshToken into a store: {}", source);
        return token;
    }

    std::shared_ptr<MeshToken> Meshes::load(content::Path const & source,
                                            material::Model::Sptr const & defaultMaterial)
    {
        std::shared_ptr<MeshToken> result = std::make_shared<MeshToken>();
        result->_mesh = std::make_unique<Mesh>(source, defaultMaterial,
                                               _contentManager, _materials, _ubo);
        MINIRE_INFO("Loading model: {}, {}", source, defaultMaterial ? "(default material)"
                                                                     : "(no default material)");
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
