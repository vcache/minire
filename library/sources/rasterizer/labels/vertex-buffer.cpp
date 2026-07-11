#include <rasterizer/labels/vertex-buffer.hpp>

#include <rasterizer/font.hpp>
#include <text/iterator.hpp>

#include <minire/text/text-format.hpp>

#include <cassert>

namespace minire::rasterizer::labels
{
    namespace
    {
        uint32_t fontCodeOf(text::Format const & s)
        {
            if (s.bold()) return 1;
            if (s.italic()) return 2;
            return 0;
        }

        size_t append(size_t disp,
                      float gx, float gy,
                      float gw, float gh,
                      text::Symbol const & symbol,
                      Font const & font,
                      std::vector<Vertex> & out)
        {
            uint32_t const fontCode = fontCodeOf(symbol);
            utils::Rect uv = font.uvRect(symbol.codePoint(), L'?');
            uv += .5f; // TODO: move that into rasterizer::Font::Font

            glm::vec4 bg(0), fg(0);
            if (!symbol.blank())
            {
                fg = symbol.foreground();
                bg = symbol.background();
                if (symbol.invertColors())
                {
                    std::swap(fg, bg);
                }
            }

            gw -= .5f;
            gh -= .5f;

            out[disp + 0] = Vertex{ glm::vec2(gx, gy + gh),
                                    glm::vec2(uv._left, uv._bottom),
                                    fg, bg, fontCode};
            out[disp + 1] = Vertex{ glm::vec2(gx, gy),
                                    glm::vec2(uv._left, uv._top),
                                    fg, bg, fontCode};
            out[disp + 2] = Vertex{ glm::vec2(gx + gw, gy),
                                    glm::vec2(uv._right, uv._top),
                                    fg, bg, fontCode};
            out[disp + 3] = Vertex{ glm::vec2(gx, gy + gh),
                                    glm::vec2(uv._left, uv._bottom),
                                    fg, bg, fontCode};
            out[disp + 4] = Vertex{ glm::vec2(gx + gw, gy),
                                    glm::vec2(uv._right, uv._top),
                                    fg, bg, fontCode};
            out[disp + 5] = Vertex{ glm::vec2(gx + gw, gy + gh),
                                    glm::vec2(uv._right, uv._bottom),
                                    fg, bg, fontCode};

            return 6;
        }

        Font const & selectFont(text::Symbol const & symbol,
                                Font const & fontRegular,
                                Font const & fontBold,
                                Font const & fontItalic)
        {
            if (symbol.bold())
            {
                if (fontBold.loaded(symbol.codePoint()))
                {
                    return fontBold;
                }
            }
            else if (symbol.italic())
            {
                if (fontItalic.loaded(symbol.codePoint()))
                {
                    return fontItalic;
                }
            }
            return fontRegular;
        }
    }

    std::vector<Vertex> buildMesh(text::FormattedString const & text,
                                  Font const & fontRegular,
                                  Font const & fontBold,
                                  Font const & fontItalic)
    {
        assert(fontRegular.glyphSize() == fontBold.glyphSize());
        assert(fontRegular.glyphSize() == fontItalic.glyphSize());
        glm::vec2 const & glyphSize = fontRegular.glyphSize();

        std::vector<Vertex> result(text.size() * 6);
        size_t offset = 0;
        text::iterate(text, glyphSize,
            [&](text::Symbol const & symbol, glm::vec2 const & positionMin,
                glm::vec2 const & glyphSize)
            {
                Font const & font = selectFont(symbol, fontRegular, fontBold, fontItalic);
                offset += append(offset, positionMin.x, positionMin.y, glyphSize.x, glyphSize.y,
                                 symbol, font, result);
                return true;
            });
        result.resize(offset);
        return result;
    }

    // VertexBuffer //

    VertexBuffer::VertexBuffer(std::vector<Vertex> const & vertices)
        : _vao()
        , _vbo(GL_ARRAY_BUFFER)
        , _vertices(vertices.size())
    {
        size_t constexpr stride = sizeof(Vertex);
        size_t pointer = 0;

        // layout(location = 0) in vec2 bznkPos;
        _vao.enableAttrib(0);
        _vao.attribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, pointer);
        pointer += sizeof(Vertex::_pos);

        // layout(location = 1) in vec2 bznkUv;
        _vao.enableAttrib(1);
        _vao.attribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, pointer);
        pointer += sizeof(Vertex::_uv);

        // layout(location = 2) in vec4 bznkFgColor;
        _vao.enableAttrib(2);
        _vao.attribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, pointer);
        pointer += sizeof(Vertex::_fgColor);

        // layout(location = 3) in vec4 bznkBgColor;
        _vao.enableAttrib(3);
        _vao.attribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, pointer);
        pointer += sizeof(Vertex::_bgColor);

        // layout(location = 4) in uint bznkFont;
        _vao.enableAttrib(4);
        _vao.attribIPointer(4, 1, GL_UNSIGNED_INT, stride, pointer);
        pointer += sizeof(Vertex::_font);

        // upload vertices data
        _vbo.bufferData(vertices.size() * sizeof(Vertex),
                        vertices.data(), GL_STATIC_DRAW);
    }
}
