#pragma once

#include <opengl/texture.hpp>

#include <memory>

namespace minire::rasterizer::filters
{
    class GaussianBlur
    {
        GaussianBlur(GaussianBlur const &) = delete;
        GaussianBlur(GaussianBlur &&) = delete;
        GaussianBlur & operator=(GaussianBlur const &) = delete;
        GaussianBlur & operator=(GaussianBlur &&) = delete;

    public:
        explicit GaussianBlur(size_t width,
                              size_t height,
                              GLint textureFormat);

        ~GaussianBlur();

        void perform(opengl::Texture const &) const;

    private:
        class Program;
        using ProgramUptr = std::unique_ptr<Program>;

        size_t const    _width;
        size_t const    _height;
        GLint const     _textureFormat;
        ProgramUptr     _program;
        opengl::Texture _temp;
    };
}