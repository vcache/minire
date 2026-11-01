#pragma once

#include <rasterizer/culled-objects.hpp>
#include <rasterizer/materials.hpp>
#include <rasterizer/mesh.hpp>
#include <opengl/shared-vertex-buffer.hpp>

#include <boost/container_hash/hash.hpp>
#include <glm/vec3.hpp>

#include <cassert>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace minire::rasterizer
{
    class DrawBatcher
    {
        struct DirectItem
        {
            UniquePrimitive const *    _uniquePrimitive;
            PrimitiveInstances const * _primitiveInstances;
            Materials::Brush const *   _brush;
        };

    public:
        template<typename BrushResolver>
        void collect(CulledPrimitives const & culledPrimitives,
                     BrushResolver && brushResolver,
                     Material::Sptr const & overrideMaterial = {})
        {
            _batches.clear();
            _directItems.clear();

            for (auto const & [uniquePrimitive, primitiveInstances] : culledPrimitives)
            {
                if (primitiveInstances.size() == 0) continue;

                Materials::Brush const & brush = brushResolver(uniquePrimitive);

                if (uniquePrimitive._mesh.hasSubBuffer(uniquePrimitive._primitiveIndex))
                {
                    auto const * subBuffer = uniquePrimitive._mesh.subBuffer(uniquePrimitive._primitiveIndex);
                    assert(subBuffer);
                    auto svb = subBuffer->sharedVertexBuffer();
                    assert(svb);

                    Material const * material = overrideMaterial
                        ? overrideMaterial.get()
                        : &uniquePrimitive._mesh.material(uniquePrimitive._primitiveIndex);

                    Key key{&brush, svb.get(), material->stateSlug()};
                    auto & batch = _batches[key];
                    batch._items.push_back(Materials::Brush::MultiDrawItem{
                        subBuffer,
                        &primitiveInstances
                    });
                    if (!batch._material)
                    {
                        batch._material = material;
                    }
                }
                else
                {
                    _directItems.push_back(DirectItem{
                        &uniquePrimitive,
                        &primitiveInstances,
                        &brush
                    });
                }
            }
        }

        void draw(glm::vec3 const & ambientLight,
                  material::TextureRefs const & directionalLightsShadowMaps,
                  material::TextureRefs const & pointLightsShadowMaps,
                  Material::Sptr const & overrideMaterial = {}) const
        {
            for (auto const & [key, batch] : _batches)
            {
                assert(key._brush);
                assert(key._svb);
                assert(batch._material);
                key._brush->drawMultiIndirect(batch._items, *key._svb, ambientLight,
                                              directionalLightsShadowMaps, pointLightsShadowMaps,
                                              *batch._material);
            }

            for (auto const & item : _directItems)
            {
                assert(item._brush);
                assert(item._uniquePrimitive);
                assert(item._primitiveInstances);
                item._brush->draw(*item._uniquePrimitive, *item._primitiveInstances,
                                  ambientLight, directionalLightsShadowMaps,
                                  pointLightsShadowMaps, overrideMaterial);
            }
        }

    private:
        struct Key
        {
            Materials::Brush const *           _brush;
            opengl::SharedVertexBuffer const * _svb;
            std::string_view                   _stateSlug;

            bool operator==(Key const & o) const
            {
                return _brush == o._brush && _svb == o._svb && _stateSlug == o._stateSlug;
            }
        };

        struct Batch
        {
            std::vector<Materials::Brush::MultiDrawItem> _items;
            Material const *                             _material = nullptr;
        };

        struct KeyHash
        {
            size_t operator()(Key const & k) const
            {
                size_t result = 0;
                boost::hash_combine(result, k._brush);
                boost::hash_combine(result, k._svb);
                boost::hash_combine(result, std::hash<std::string_view>{}(k._stateSlug));
                return result;
            }
        };

        std::unordered_map<Key, Batch, KeyHash> _batches;
        std::vector<DirectItem>                 _directItems;
    };
}
