#include <rasterizer/label.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/rect.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>
#include <rasterizer/font.hpp>
#include <rasterizer/fonts.hpp>
#include <text/iterator.hpp>

#include <glm/common.hpp> // for gln::value_ptr

#include <cassert>
#include <cmath>
#include <limits>

namespace minire::rasterizer
{
    namespace
    {
        glm::vec2 pixelFix(glm::vec2 const & in)
        {
            return glm::vec2(
                std::floor(in.x) + .5f,
                std::floor(in.y) + .5f
            );
        }
    }

    static const char * kVertShader = R"(
        #version 330 core

        layout(location = 0) in vec2 bznkPos;
        layout(location = 1) in vec2 bznkUv;
        layout(location = 2) in vec4 bznkFgColor;
        layout(location = 3) in vec4 bznkBgColor;
        layout(location = 4) in uint bznkFont;

        uniform mat4 bznkProj;
        uniform vec2 bznkPosition;

        out vec2 bznkFragPos;
        out vec2 bznkFragUv;
        flat out vec4 bznkFragFgColor;
        flat out vec4 bznkFragBgColor;
        flat out uint bznkFragFont;

        void main()
        {
            gl_Position = bznkProj *
                          vec4(bznkPosition + bznkPos, 0.0, 1.0);
            bznkFragUv = bznkUv;
            bznkFragFgColor = bznkFgColor;
            bznkFragBgColor = bznkBgColor;
            bznkFragFont = bznkFont;
        }
    )";

    static const char * kFragShader = R"(
        #version 330 core

        in vec2 bznkFragPos;
        in vec2 bznkFragUv;
        flat in vec4 bznkFragFgColor;
        flat in vec4 bznkFragBgColor;
        flat in uint bznkFragFont;

        uniform sampler2D bznkFonts[3];

        out vec4 bznkOutColor;

        void main()
        {
            ivec2 offset = ivec2(bznkFragUv);
            float fgFactor = texelFetch(bznkFonts[bznkFragFont], offset, 0).r;

            //float fgFactor = texture(bznkFonts[bznkFragFont], bznkFragUv).r;
            float bgFactor = 1.0 - fgFactor;
            bznkOutColor = bznkFragBgColor * bgFactor
                         + bznkFragFgColor * fgFactor;
        }
    )";

    class Label::Program
    {
    public:
        Program()
            : _program({
                std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, kVertShader),
                std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader)
            })
            , _fontsUniform(_program.getUniformLocation("bznkFonts"))
            , _projUniform(_program.getUniformLocation("bznkProj"))
            , _positionUniform(_program.getUniformLocation("bznkPosition"))
        {}

        void use() const { _program.use(); }

        void setFontsUniform(std::array<GLint, 3> const & v) const
        {
            MINIRE_GL(glUniform1iv, _fontsUniform, v.size(), v.data());
        }

        void setProjUniform(glm::mat4 const & m) const
        {
            MINIRE_GL(glUniformMatrix4fv, _projUniform, 1, GL_FALSE, glm::value_ptr(m));
        }

        void setPositionUniform(glm::vec2 const & v) const
        {
            MINIRE_GL(glUniform2f, _positionUniform, v.x, v.y);
        }

    public:
        static Program const & instance()
        {
            static const Program kProgram;
            return kProgram;
        }

    private:
        opengl::Program _program;
        GLint           _fontsUniform;
        GLint           _projUniform;
        GLint           _positionUniform;
    };

    // NOTE: Clients must align data elements consistent with the 
    //       requirements of the client platform, with an additional
    //       base-level requirement that an offset within a buffer to
    //       a datum comprising N be a multiple of N.        
    struct Vertex
    {
        glm::vec2 _pos;
        glm::vec2 _uv;
        glm::vec4 _fgColor;
        glm::vec4 _bgColor;
        uint32_t  _font;
    };

    class Label::Buffer
    {
        size_t append(size_t disp,
                      float gx, float gy,
                      float gw, float gh,
                      text::TextFormat const & format,
                      wchar_t codePoint,
                      Font const & font,
                      glm::vec2 const & clip,
                      std::vector<Vertex> & out)
        {
            uint32_t const fontCode = fontCodeOf(format);
            utils::Rect uv = font.uvRect(codePoint, L'?');
            uv += .5f; // TODO: move that into rasterizer::Font::Font

            glm::vec4 bg(0), fg(0);
            if (!format.blank())
            {
                fg = format.foreground();
                bg = format.background();
                if (format.invertColors())
                {
                    std::swap(fg, bg);
                }
            }

            gw -= .5f;
            gh -= .5f;

            uv._right -= clip.x;
            uv._bottom -= clip.y;
            gw -= clip.x;
            gh -= clip.y;

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

        static uint32_t fontCodeOf(text::TextFormat const & s)
        {
            if (s.bold()) return 1;
            if (s.italic()) return 2;
            return 0;
        }

        static Font const & selectFont(wchar_t codePoint,
                                       text::TextFormat const & format,
                                       Font const & fontRegular,
                                       Font const & fontBold,
                                       Font const & fontItalic)
        {
            if (format.bold())
            {
                if (fontBold.loaded(codePoint))
                {
                    return fontBold;
                }
            }
            else if (format.italic())
            {
                if (fontItalic.loaded(codePoint))
                {
                    return fontItalic;
                }
            }
            return fontRegular;
        }

    public:
        void update(text::FormattedString const & text,
                    Font const & fontRegular,
                    Font const & fontBold,
                    Font const & fontItalic,
                    std::optional<glm::vec2> const & maxSize)
        {
            _vao->bind();

            assert(fontRegular.glyphSize() == fontBold.glyphSize());
            assert(fontRegular.glyphSize() == fontItalic.glyphSize());
            glm::vec2 const & glyphSize = fontRegular.glyphSize();

            _vertices.resize(text.size() * 6);
            size_t offset = 0;

            text::iterate(text, glyphSize,
                [&offset, this, &fontRegular, &fontBold, &fontItalic, &maxSize]
                (text::TextFormat const & format, wchar_t codePoint,
                 glm::vec2 const & position, glm::vec2 const & glyphSize)
                {
                    glm::vec2 clip(0);
                    if (maxSize)
                    {
                        if (position.y >= maxSize->y) return false;
                        if (position.x >= maxSize->x) return true;

                        assert(position.x < maxSize->x);
                        clip = glm::max(glm::vec2(0), position + glyphSize - *maxSize);
                    }

                    Font const & font = selectFont(codePoint, format, fontRegular, fontBold, fontItalic);
                    offset += append(offset, position.x, position.y, glyphSize.x, glyphSize.y,
                                     format, codePoint, font, clip, _vertices);
                    return true;
                });
            _vertices.resize(offset);

            _vbo.bufferData(_vertices.size() * sizeof(Vertex),
                            _vertices.data(), GL_STATIC_DRAW);
        }

        void draw() const
        {
            _vao->bind();
            MINIRE_GL(glDrawArrays, GL_TRIANGLES, 0, _vertices.size());
        }

        Buffer()
            : _vao(std::make_shared<opengl::VAO>())
            , _vbo(_vao, GL_ARRAY_BUFFER)
        {
            size_t const stride = sizeof(Vertex);
            size_t pointer = 0;

            // layout(location = 0) in vec2 bznkPos;
            _vao->enableAttrib(0);
            _vao->attribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, pointer);
            pointer += sizeof(Vertex::_pos);

            // layout(location = 1) in vec2 bznkUv;
            _vao->enableAttrib(1);
            _vao->attribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, pointer);
            pointer += sizeof(Vertex::_uv);

            // layout(location = 2) in vec4 bznkFgColor;
            _vao->enableAttrib(2);
            _vao->attribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, pointer);
            pointer += sizeof(Vertex::_fgColor);

            // layout(location = 3) in vec4 bznkBgColor;
            _vao->enableAttrib(3);
            _vao->attribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, pointer);
            pointer += sizeof(Vertex::_bgColor);

            // layout(location = 4) in uint bznkFont;
            _vao->enableAttrib(4);
            _vao->attribIPointer(4, 1, GL_UNSIGNED_INT, stride, pointer);
            pointer += sizeof(Vertex::_font);
        }

    private:
        opengl::VAO::Sptr   _vao;
        opengl::VBO         _vbo;
        std::vector<Vertex> _vertices;
    };

    Label::Label(Fonts const & fonts,
                 text::FormattedString const & text,
                 int z, bool visible)
        : Drawable(z)
        , _fonts(fonts)
        , _text(text)
        , _position(0.0)
        , _program(Program::instance())
        , _buffer(std::make_unique<Buffer>())
        , _invalidated(true)
        , _visible(visible)
    {}

    Label::~Label() = default;

    void Label::setFontFace(content::Id const & fontName,
                            content::Manager & contentManager)
    {
        auto lease = contentManager.borrow(fontName);
        assert(lease);
        setFontFace(lease->as<models::FontFace>());
    }

    void Label::setFontFace(models::FontFace const & fontData)
    {
        _fontRegular = _fonts.get(fontData._regular);
        _fontBold = _fonts.get(fontData._bold);
        _fontItalic = _fonts.get(fontData._italic);

        assert(_fontRegular);
        assert(_fontBold);
        assert(_fontItalic);

        glm::vec2 glyphSize = _fontRegular->glyphSize();
        MINIRE_INVARIANT(glyphSize == _fontBold->glyphSize(),
                         "only monospaced fonts are supported, "
                         "but fonts differ in size: {}, {}",
                         fontData._regular, fontData._bold);
        MINIRE_INVARIANT(glyphSize == _fontItalic->glyphSize(),
                         "only monospaced fonts are supported, "
                         "but fonts differ in size: {}, {}",
                         fontData._regular, fontData._italic);
        MINIRE_INVARIANT(fontData._glyphWidth == glyphSize.x &&
                         fontData._glyphHeight == glyphSize.y,
                         "glyph sizes differ from models: {}x{} != {}x{}",
                         fontData._glyphWidth, fontData._glyphHeight,
                         glyphSize.x, glyphSize.y);

        _invalidated = true;
    }

    void Label::setMaxSize(std::optional<glm::vec2> const & maxSize)
    {
        _maxSize = maxSize;
        _invalidated = true;
    }

    void Label::setText(text::FormattedString const & text)
    {
        _text = text;
        _invalidated = true;
    }

    text::FormattedString const & Label::text() const
    {
        return _text;
    }

    void Label::draw(glm::mat4 const & projection) const
    {
        static const std::array<GLint, 3> kTextureUnits{0, 1, 2};

        if (_invalidated)
        {
            revalidate();
        }

        assert(_buffer);
        assert(_fontRegular);
        assert(_fontBold);
        assert(_fontItalic);

        _program.use();

        MINIRE_GL(glActiveTexture, GL_TEXTURE0);
        _fontRegular->bind();

        MINIRE_GL(glActiveTexture, GL_TEXTURE1);
        _fontBold->bind();

        MINIRE_GL(glActiveTexture, GL_TEXTURE2);
        _fontItalic->bind();

        _program.setFontsUniform(kTextureUnits);
        _program.setProjUniform(projection);
        _program.setPositionUniform(_position);

        _buffer->draw();
    }

    void Label::setPosition(glm::vec2 position)
    {
        _position = pixelFix(position);
    }

    void Label::revalidate() const
    {
        assert(_buffer);
        assert(_fontRegular);
        assert(_fontBold);
        assert(_fontItalic);

        _buffer->update(_text,
                        *_fontRegular,
                        *_fontBold,
                        *_fontItalic,
                        _maxSize);

        _invalidated = false;
    }
}
