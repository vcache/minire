#include <gpu/render/font.hpp>

#include <minire/errors.hpp>
#include <minire/formats/bdf.hpp>
#include <minire/logging.hpp>
#include <opengl.hpp>

#include <cassert>

namespace minire::gpu::render
{
    Font::Font(formats::Bdf const & bdf)
        : _texture(GL_TEXTURE_2D)
    {
        auto const bbox = bdf.bbox();

        // glyph parameters
        _glyphWidth = bbox._w;
        _glyphHeight = bbox._h;
        
        // allocate atlas
        using AtlasPixel = uint8_t;
        size_t atlasWidth = minimalSide(bdf.loadedChars());
        size_t atlasHeight = atlasWidth;
        size_t atlasBytes = atlasWidth * atlasHeight * sizeof(AtlasPixel);
        std::vector<AtlasPixel> atlasPixels(atlasBytes, 0);

        size_t stX = 0, stY = 0;
        _uvMapping.resize(bdf.maxEncoding() + 1,
                          std::make_pair(false, utils::Rect()));
        for(size_t i = 0; i <= bdf.maxEncoding(); ++i)
        {
            auto const & glyph = bdf.find(i);
            if (!glyph.loaded()) continue;

            glyph.render<AtlasPixel, 0xFF, 0x00>(atlasPixels.data(),
                                                 atlasWidth,
                                                 atlasHeight,
                                                 stX, stY);

            /*
            Rect uvRect((float(stX) - .5) * float(atlasWidth),
                        (float(stY) - .5) * float(atlasHeight),
                        (float(stX + bbox._w - 1) + .5) * float(atlasWidth),
                        (float(stY + bbox._h - 1) + .5) * float(atlasHeight));
            */
            utils::Rect uvRect(stX, stY,
                               stX + bbox._w - 1,
                               stY + bbox._h - 1);
            _uvMapping[i] = std::make_pair(true, uvRect);

            stX += bbox._w;
            if (stX + bbox._w > atlasWidth)
            {
                stX = 0;
                stY += bbox._h;
                if (stY + bbox._h > atlasHeight)
                {
                    MINIRE_THROW("bad dimension stX = {}, stY = {}, atlas {}x{}, font {}x{}",
                                 stX, stY, atlasWidth, atlasHeight, bbox._w, bbox._h);
                }
            }
        }

        // load atlas into GPU
        _texture.bind();
        MINIRE_GL(glTexStorage2D, GL_TEXTURE_2D, 1, GL_R8, atlasWidth, atlasHeight);
        MINIRE_GL(glTexSubImage2D, GL_TEXTURE_2D, 0, 0, 0, atlasWidth, atlasHeight,
                  GL_RED, GL_UNSIGNED_BYTE, atlasPixels.data());

        // texture parameters
        _texture.parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        _texture.parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        _texture.parameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        _texture.parameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        MINIRE_DEBUG("texture for {}: {} ({}x{})",
                     bdf.filename(), _texture.id(), atlasWidth, atlasHeight);
    }

    size_t Font::minimalSide(size_t chars) const
    {
        static constexpr int kMax = 18;

        int i;
        for(i = 0; i < kMax; ++i)
        {
            size_t const side = size_t(1) << i;

            size_t const charsPerVert = side / _glyphWidth;
            size_t const charsPerHoriz = side / _glyphHeight;
            if (charsPerVert * charsPerHoriz >= chars)
            {
                break;
            }
        }

        if (i == kMax)
        {
            MINIRE_THROW("too large atlas: {}, {}x{}",
                         chars, _glyphWidth, _glyphHeight);
        }

        return 1 << i;
    }

    void Font::bind() const
    {
        _texture.bind();
    }

    utils::Rect const & Font::uvRect(size_t codePoint, size_t fallback) const
    {
        return uvRect(loaded(codePoint) ? codePoint : fallback);
    }

    utils::Rect const & Font::uvRect(size_t codePoint) const
    {
        assert(loaded(codePoint));
        return _uvMapping[codePoint].second;
    }

    bool Font::loaded(size_t codePoint) const
    {
        return codePoint < _uvMapping.size()
            && _uvMapping[codePoint].first;
    }
}
