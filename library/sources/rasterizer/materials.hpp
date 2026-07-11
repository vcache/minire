#pragma once

#include <material/types.hpp>
#include <opengl/program.hpp>
#include <rasterizer/culled-objects.hpp>
#include <rasterizer/instanced-buffers.hpp>
#include <rasterizer/locations-allocator.hpp>
#include <rasterizer/textures.hpp>

#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>
#include <minire/utils/std-pair-hash.hpp>

#include <nlohmann/json_fwd.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace minire::rasterizer { class Ubo; }

namespace minire::rasterizer
{
    // TODO: maybe a cache can be localized in Material, so that
    //       the whole Materials class becomes useless.
    class Materials
    {
        struct TemplateRenderingOutput;

    public:
        explicit Materials(Textures const & textures,
                           Ubo const & ubo,
                           InstancedBuffersPool & instancedBuffersPool);

        material::Locations const & locations() const { return _attribLocations; }

    public:
        class Brush
        {
        public:
            using Sptr = std::shared_ptr<Brush>;
            using Wptr = std::weak_ptr<Brush>;

            explicit Brush(Materials const & materials,
                           Textures const & textures,
                           Ubo const & ubo,
                           InstancedBuffersPool & instancedBuffersPool,
                           TemplateRenderingOutput const & tro,
                           material::Shaders && sources,
                           std::unique_ptr<nlohmann::json> && templateParams,
                           models::MeshFeatures const & meshFeatures,
                           Material::Sptr const & material);

            // TODO: some parameters can be optional!
            void draw(UniquePrimitive const &,
                      PrimitiveInstances const &,
                      glm::vec3 const & ambientLight,
                      material::TextureRefs const & directionalLightsShadowMaps,
                      material::TextureRefs const & pointLightsShadowMaps) const;

            Material::Sptr const & material() const { return _material; }

        private:
            class TextureUnitHelper;

            void setupBuiltinUniforms(TextureUnitHelper & textureUnitHelper,
                                      glm::vec3 const & ambientLight,
                                      material::TextureRefs const & directionalLightsShadowMaps,
                                      material::TextureRefs const & pointLightsShadowMaps) const;

            void setupCustomUniforms(TextureUnitHelper & textureUnitHelper) const;

        private:
            Materials const               & _materials;
            Textures const                & _textures;
            InstancedBuffersPool &          _instancedBuffersPool;
            material::Shaders const         _sources;
            std::unique_ptr<nlohmann::json> _templateParams;
            models::MeshFeatures const      _meshFeatures;
            Material::Sptr                  _material;
            opengl::Program                 _program;

            // Builtin Uniform prescence

            GLint const                     _directionalLightsShadowMapsUniform;
            GLint const                     _pointLightsShadowMapsUniform;
            GLint const                     _ambientLightUniform;

            // Custom Uniforms

            struct UserUniformMeta
            {
                Textures::Texture::Sptr _texture;
                content::Id             _contentId;
                int const               _location;
            };

            mutable material::UserUniforms       _userUniforms;
            mutable std::vector<UserUniformMeta> _userUniformMeta;
        };

        Brush::Sptr getBrush(models::MeshFeatures const &,
                             Material::Sptr const &) const;

    private:
        std::unique_ptr<nlohmann::json>
        makeBasicTemplateParams(models::MeshFeatures const &,
                                std::string const & slug,
                                std::string const & name) const;

    private:
        // NOTE: The should be dependant on basic template parameters,
        //       but since they are defined by MeshFeatures (everything else is const),
        //       and it is expensive to use as a key, here MeshFeatures used.
        using Key = std::pair<std::string /* material's slug */,
                              models::MeshFeatures>;
        using Store = std::unordered_map<Key, Brush::Wptr>;

        Textures const          & _textures;
        Ubo const               & _ubo;
        InstancedBuffersPool    & _instancedBuffersPool;

        // Vertex Attributes locations,
        // must be the same for all brushes

        LocationsAllocator        _locationsAllocator;
        material::Locations const _attribLocations;
        GLint const               _modelAttrib;
        GLint const               _emissiveFactorAttrib;
        GLint const               _meshIdAttrib;

        mutable Store             _store;

        friend class Brush;
    };
}