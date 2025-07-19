#include <rasterizer/textures.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>

#include <opengl.hpp>

#include <glm/glm.hpp>

#include <cassert>

namespace minire::rasterizer
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
                               models::Sampler const & sampler,
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
        _texture.parameteri(GL_TEXTURE_MIN_FILTER, sampler._minFilter);
        _texture.parameteri(GL_TEXTURE_MAG_FILTER, sampler._magFilter);
        _texture.parameteri(GL_TEXTURE_WRAP_S, sampler._wrapS);
        _texture.parameteri(GL_TEXTURE_WRAP_T, sampler._wrapT);
    }

    Textures::Texture::Sptr
    Textures::get(content::Manager & contentManager,
                  content::Id const & id,
                  models::Sampler const & sampler,
                  Cache & cache,
                  bool mipmaps)
    {
        Key key(id, sampler);

        auto it = cache.find(key);
        if (it == cache.cend())
        {
            auto lease = contentManager.borrow(id);
            assert(lease);
            models::Image::Sptr image = lease->as<models::Image::Sptr>();
            MINIRE_INVARIANT(image, "no valid image inside an asset: {}", id);
            auto texture = std::make_shared<Texture>(*image, sampler, mipmaps);
            auto [newIt, inserted]  = cache.emplace(key, texture);
            MINIRE_INVARIANT(inserted, "failed to cache a texture");
            it = newIt;
        }

        return it->second;
    }
}
