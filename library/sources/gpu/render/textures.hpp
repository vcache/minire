#pragma once

#include <minire/content/id.hpp>

#include <opengl/texture.hpp>

#include <memory>
#include <unordered_map>

namespace minire::content { class Manager; }
namespace minire::models { struct Image; }

namespace minire::gpu::render
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
        explicit Textures(content::Manager &);

        // use this both for preload and for getting ptr
        Texture::Sptr get(content::Id const &) const;

        Texture::Sptr getNoMipmap(content::Id const &) const;

    private:
        using Cache = std::unordered_map<content::Id, Texture::Sptr>;

        static Texture::Sptr get(content::Manager &,
                                 content::Id const &,
                                 Cache &,
                                 bool mipmaps);

    private:
        content::Manager       & _contentManager;

        mutable Cache            _cache;
        mutable Cache            _cacheNoMipmap;
    };
}
