#pragma once

#include <minire/content/id.hpp>
#include <minire/models/sampler.hpp>
#include <minire/models/texture-handle.hpp>

#include <opengl/texture.hpp>
#include <rasterizer/textures/id.hpp>

#include <memory>

namespace minire::content { class Manager; }
namespace minire::models { struct Image; }

namespace minire::rasterizer
{
    class Resources;

    class Textures
        : public models::TextureResolver
    {
    public:
        class Texture
            : public models::TextureHandle
        {
        private:
            Texture() = delete;

            Texture(Texture const &) = delete;
            Texture & operator=(Texture const &) = delete;

            Texture(Texture &&) = default;
            Texture & operator=(Texture &&) = default;

        public:
            using Sptr = std::shared_ptr<Texture>;

            // NOTE: dont' use it directly (who knows what could happen!)
            explicit Texture(models::Image const &,
                             models::Sampler const &,
                             bool mipmaps);

            void bind() const override { _texture.bind(); }

            size_t width() const { return _width; }

            size_t height() const { return _height; }

        private:
            opengl::Texture _texture;
            size_t          _width;
            size_t          _height;

            friend class Textures;
        };

    public:
        explicit Textures(content::Manager & contentManager,
                          Resources & resources)
            : _contentManager(contentManager)
            , _resources(resources)
        {}

        // use this both for preload and for getting ptr
        // TODO: why is this a const-method?
        Texture::Sptr get(textures::Id const &) const;

        Texture::Sptr get(content::Id const & contentId,
                          models::Sampler const & sampler = {},
                          bool hasMipMaps = true) const
        {
            return get(textures::Id(contentId, sampler, hasMipMaps));
        }

        Texture::Sptr get(content::MaybeId const & contentId,
                          models::Sampler const & sampler = {},
                          bool hasMipMaps = true) const
        {
            return contentId ? get(*contentId, sampler, hasMipMaps)
                             : Texture::Sptr();
        }

        models::TextureHandle::Sptr resolve(content::Id const & textureId,
                                            models::Sampler const & sampler) const override;

    private:
        content::Manager & _contentManager;
        Resources        & _resources;
    };
}
