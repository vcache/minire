#include <gpu/render/textures.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>

#include <opengl.hpp>

#include <glm/glm.hpp>

#include <cassert>

namespace minire::gpu::render
{
    namespace
    {
        GLsizei maxMipMaps(size_t w, size_t h)
        {
            size_t const side = std::min(w, h);
            return glm::floor(glm::log2(static_cast<float>(side))) - 1;        
        }
    }

    Textures::Texture::Texture(models::Image const & image,
                               bool const mipmaps)
        : _texture(GL_TEXTURE_2D) // TODO: maybe GL_TEXTURE_2D_ARRAY will be more efficient
    {
        _width = image._width;
        _height = image._height;

        _texture.bind();

        // allocate storage
        GLsizei levels = mipmaps ? maxMipMaps(_width, _height) : 1;
        MINIRE_GL(glTexStorage2D,
                  GL_TEXTURE_2D,
                  levels,
                  opengl::toInternalFormat(image._format),
                  _width, _height);

        // upload pixel data
        MINIRE_GL(glTexSubImage2D,
                  GL_TEXTURE_2D,
                  0, 0, 0,
                  _width, _height,
                  opengl::toFormat(image._format),
                  GL_UNSIGNED_BYTE,
                  image._data);

        if (mipmaps)
        {
            MINIRE_GL(glGenerateMipmap, GL_TEXTURE_2D);
        }

        // texture parameters
        _texture.parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        _texture.parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        _texture.parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        _texture.parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    Textures::Textures(content::Manager & contentManager)
        : _contentManager(contentManager)
    {}

    Textures::Texture::Sptr
    Textures::get(content::Id const & id) const
    {
        return get(_contentManager, id, _cache, true);
    }

    Textures::Texture::Sptr
    Textures::getNoMipmap(content::Id const & id) const
    {
        return get(_contentManager, id, _cacheNoMipmap, false);
    }

    Textures::Texture::Sptr
    Textures::get(content::Manager & contentManager,
                  content::Id const & id,
                  Cache & cache,
                  bool mipmaps)
    {
        auto const it = cache.find(id);
        if (it == cache.cend())
        {
            auto lease = contentManager.borrow(id);
            assert(lease);
            models::Image::Sptr image = lease->as<models::Image::Sptr>();
            MINIRE_INVARIANT(image, "no valid image inside an asset: {}", id);
            auto texture = std::make_shared<Texture>(*image, mipmaps);
            cache.emplace(id, texture);
            return texture;
        }
        return it->second;
    }
}
