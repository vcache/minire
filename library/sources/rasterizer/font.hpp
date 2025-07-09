#pragma once

#include <minire/utils/rect.hpp>

#include <opengl/texture.hpp>

#include <glm/vec2.hpp>

#include <cstddef>
#include <vector>
#include <utility>

namespace minire::formats { class Bdf; }

namespace minire::rasterizer
{
    class Font
    {
    public:
        explicit Font(formats::Bdf const & bdf);

        size_t glyphWidth() const { return _glyphWidth; }

        size_t glyphHeight() const { return _glyphHeight; }

        glm::vec2 glyphSize() const { return glm::vec2(_glyphWidth, _glyphHeight); }

        // NOTE: safe to for any codePoint
        utils::Rect const & uvRect(size_t codePoint, size_t fallback) const;

        void bind() const;

        bool loaded(size_t codePoint) const;

    private:
        size_t minimalSide(size_t chars) const;

        // NOTE: UB in case of codePoint not loaded
        utils::Rect const & uvRect(size_t codePoint) const;

    private:
        using UvMapping = std::vector<std::pair<bool, utils::Rect>>;

        opengl::Texture _texture;
        size_t          _glyphWidth;    // in pixels
        size_t          _glyphHeight;   // in pixels
        UvMapping       _uvMapping;     // char code to UVs
    };
}
