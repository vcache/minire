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
}
