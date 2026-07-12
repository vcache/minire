#include <rasterizer/textures.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>

#include <opengl.hpp>
#include <rasterizer/resources.hpp>

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
    Textures::get(textures::Id const & key) const
    {
        // Look up in cache
        if (std::any const & cached = _resources.find(key);
            cached.has_value())
        {
            return std::any_cast<Texture::Sptr>(cached);
        }

        // Build a new one
        auto lease = _contentManager.borrow(key._contentId);
        assert(lease);

        models::Image::Sptr image = lease->as<models::Image::Sptr>();
        MINIRE_INVARIANT(image, "no valid image inside an asset: {}", key._contentId);
        auto texture = std::make_shared<Texture>(*image, key._sampler, key._hasMipMaps);

        // Put in a resource cache
        _resources.insert(key, texture);

        return texture;
    }

    models::TextureHandle::Sptr Textures::resolve(content::Id const & textureId,
                                                  models::Sampler const & sampler) const
    {
        Texture::Sptr const & texture = get(textureId, sampler);
        assert(texture);
        return texture;
    }
}
