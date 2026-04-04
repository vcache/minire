#include <opengl/texture.hpp>

#include <minire/errors.hpp>

namespace minire::opengl
{
    GLenum toInternalFormat(models::Image::Format format)
    {
        switch(format)
        {
            case models::Image::Format::kRGB: return GL_RGB8;
            case models::Image::Format::kRGBA: return GL_RGBA8;
            case models::Image::Format::kGrayscale: return GL_R8;
            default: MINIRE_THROW("image format not supported: {}", int(format));
        }
    }

    GLenum toFormat(models::Image::Format format)
    {
        switch(format)
        {
            case models::Image::Format::kRGB: return GL_RGB;
            case models::Image::Format::kRGBA: return GL_RGBA;
            case models::Image::Format::kGrayscale: return GL_RED;
            default: MINIRE_THROW("image format not supported: {}", int(format));
        }
    }

    std::pair<size_t, size_t> Texture::size(GLint miplevel) const
    {
        GLint width = 0, height = 0;

        bind();
        MINIRE_GL(glGetTexLevelParameteriv, GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &width);
        MINIRE_GL(glGetTexLevelParameteriv, GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &height);

        MINIRE_INVARIANT(width >= 0 && height >= 0, "bad dimensions for miplevel={}: {}x{}",
                         miplevel, width, height);

        return std::make_pair(static_cast<size_t>(width),
                              static_cast<size_t>(height));
    }

    GLint Texture::internalFormat() const
    {
        bind();
        GLint componentType = 0;
        MINIRE_GL(glGetTexLevelParameteriv, GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &componentType);
        return componentType;
    }
}
