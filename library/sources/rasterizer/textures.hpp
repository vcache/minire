#pragma once

#include <minire/content/id.hpp>
#include <minire/models/sampler.hpp>
#include <minire/utils/std-pair-hash.hpp>

#include <opengl/texture.hpp>

#include <memory>
#include <unordered_map>

namespace minire::content { class Manager; }
namespace minire::models { struct Image; }

namespace minire::rasterizer
{
    class Textures
    {
    public:
        class Texture
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

            void bind() const { _texture.bind(); }

            size_t width() const { return _width; }

            size_t height() const { return _height; }

        private:
            opengl::Texture _texture;
            size_t          _width;
            size_t          _height;

            friend class Textures;
        };

    public:
        explicit Textures(content::Manager & contentManager)
            : _contentManager(contentManager)
        {}

        // use this both for preload and for getting ptr
        Texture::Sptr get(content::Id const & id,
                          models::Sampler const & sampler = {}) const
        {
            return get(_contentManager, id, sampler, _cache, true);
        }

        Texture::Sptr get(content::MaybeId const & id,
                          models::Sampler const & sampler = {}) const
        {
            return id ? get(*id, sampler) : Texture::Sptr();
        }

        Texture::Sptr getNoMipmap(content::Id const & id,
                                  models::Sampler const & sampler = {}) const
        {
            return get(_contentManager, id, sampler, _cacheNoMipmap, false);
        }

        Texture::Sptr getNoMipmap(content::MaybeId const & id,
                                  models::Sampler const & sampler = {}) const
        {
            return id ? get(*id, sampler) : Texture::Sptr();
        }

    private:
        using Key = std::pair<content::Id, models::Sampler>;
        using Cache = std::unordered_map<Key, Texture::Sptr>;

        static Texture::Sptr get(content::Manager &,
                                 content::Id const &,
                                 models::Sampler const &,
                                 Cache &,
                                 bool mipmaps);

        // TODO: move mipmap into Sampler and merge _cache w/ _cacheNoMipmap

    private:
        content::Manager       & _contentManager;

        mutable Cache            _cache;
        mutable Cache            _cacheNoMipmap;
    };
}
