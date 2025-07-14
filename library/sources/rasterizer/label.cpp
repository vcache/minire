#include <rasterizer/label.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/utils/rect.hpp>

#include <rasterizer/font.hpp>
#include <rasterizer/fonts.hpp>
#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>
#include <utils/sparse-range.hpp>

#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr

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
        void setupSymbol(size_t disp,
                         float gx, float gy,
                         float gw, float gh,
                         text::Symbol const & symbol,
                         Font const & font,
                         uint32_t fontCode,
                         bool const isCursor,
                         std::vector<Vertex> & out)
        {
            utils::Rect /*const &*/ uv = font.uvRect(symbol.codePoint(), L'?');

            if (true) // TODO: move that into rasterizer::Font::Font
            {
                uv._left += .5f;
                uv._right += .5f;
                uv._top += .5f;
                uv._bottom += .5f;
            }

            glm::vec4 fg = symbol.foreground();
            glm::vec4 bg = symbol.background();

            if (symbol.blank())
            {
                bg = fg = glm::vec4(0);
            }
            else
            {
                /*
                    inv cur | res
                    --------+----
                     0   0  |  0
                     0   1  |  1
                     1   0  |  1
                     1   1  |  0
                */
                if (symbol.invertColors() != isCursor)
                {
                    std::swap(fg.x, bg.x);
                    std::swap(fg.y, bg.y);
                    std::swap(fg.z, bg.z);
                    if (isCursor) fg.w = bg.w = 1.0f;
                }
            }

            //gx -= .5f;
            //gy -= .5f;
            gw -= .5f;
            gh -= .5f;

            out[disp + 0] = Vertex{ glm::vec2(gx, gy + gh),
                                    glm::vec2(uv._left, uv._top),
                                    fg, bg, fontCode};
            out[disp + 1] = Vertex{ glm::vec2(gx, gy), 
                                    glm::vec2(uv._left, uv._bottom),
                                    fg, bg, fontCode};
            out[disp + 2] = Vertex{ glm::vec2(gx + gw, gy),
                                    glm::vec2(uv._right, uv._bottom),
                                    fg, bg, fontCode};
            out[disp + 3] = Vertex{ glm::vec2(gx, gy + gh),
                                    glm::vec2(uv._left, uv._top),
                                    fg, bg, fontCode};
            out[disp + 4] = Vertex{ glm::vec2(gx + gw, gy),
                                    glm::vec2(uv._right, uv._bottom),
                                    fg, bg, fontCode};
            out[disp + 5] = Vertex{ glm::vec2(gx + gw, gy + gh),
                                    glm::vec2(uv._right, uv._top),
                                    fg, bg, fontCode};
        }

        std::pair<size_t, size_t> // in byte (first, past-the-end)
        update(Symbols const & symbols,
               Dirty const & /*dirty*/,
               Font const & fontRegular,
               Font const & fontBold,
               Font const & fontItalic,
               Label::Cursor const & cursor,
               glm::vec2 const & glyphSize,
               size_t const col,
               size_t const row)
        {
            size_t const disp = col * 6 + row * 6 * symbols.cols();

            text::Symbol const & symbol = symbols.at(row, col);
            size_t const gw = glyphSize.x;
            size_t const gh = glyphSize.y;

            bool const isCursor = cursor._shown
                               && cursor._row == row
                               && cursor._column == col;

            size_t const fontCode = fontCodeOf(symbol);
            Font const * font = &fontRegular;
            if (symbol.bold() && fontBold.loaded(symbol.codePoint()))
                font = &fontBold;
            if (symbol.italic() && fontItalic.loaded(symbol.codePoint()))
                font = &fontItalic;

            setupSymbol(disp, col * gw, row * gh, gw, gh,
                        symbol, *font, fontCode, isCursor,
                        _vertices);

            return std::make_pair(disp * sizeof(Vertex),
                                  (disp + 6) * sizeof(Vertex));
        }

        static size_t fontCodeOf(text::Symbol const & s)
        {
            if (s.bold()) return 1;
            if (s.italic()) return 2;
            return 0;
        }

    public:
        void update(Symbols const & symbols,
                    Dirty const & dirty,
                    Font const & fontRegular,
                    Font const & fontBold,
                    Font const & fontItalic,
                    Label::Cursor const & cursor,
                    glm::vec2 const & glyphSize)
        {
            size_t const rows = symbols.rows();
            size_t const cols = symbols.cols();
            size_t required = rows * cols * 6 * sizeof(Vertex);

            _vao->bind();
            assert(std::numeric_limits<GLsizeiptr>::max() > required);
            if (static_cast<GLsizeiptr>(required) != _vbo.size())
            {   // size changed, rebuild whole buffer
                _vertices.resize(rows * cols * 6);
                for(size_t col(0); col < cols; ++col)
                for(size_t row(0); row < rows; ++row)
                {
                    update(symbols, dirty,
                           fontRegular, fontBold, fontItalic,
                           cursor, glyphSize, col, row);
                }

                // TODO: maybe reuse existing VBO if it is large enough
                _vbo.bufferData(required, _vertices.data(), GL_STATIC_DRAW);
            }
            else if (!dirty.empty())
            {   // size not changed, just update dirties
                utils::SparseRange<size_t> updates; // in bytes
                for (auto const & d: dirty)
                {
                    auto range = update(symbols, dirty, 
                                        fontRegular, fontBold, fontItalic,
                                        cursor, glyphSize, d.second, d.first);
                    updates.insert(range.first, range.second);
                }
                for(auto const & range : updates.tighten())
                {
                    assert(range.first < range.second);
                    size_t const offset = range.first;
                    size_t const size = range.second - range.first;
                    _vbo.bufferSubData(offset, size,
                        reinterpret_cast<uint8_t*>(_vertices.data()) + offset);
                }
            }
        }

        void draw() const
        {
            _vao->bind();
            _vbo.bind();
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
                 int z, bool visible)
        : Drawable(z)
        , _fonts(fonts)
        , _symbols()
        , _position(0.0)
        , _program(Program::instance())
        , _buffer(std::make_unique<Buffer>())
        , _invalidated(true)
        , _visible(visible)
    {}

    Label::~Label() = default;

    void Label::resize(size_t rows, size_t cols)
    {
        _symbols.resize(rows, cols);
        _invalidated = true;
    }

    void Label::setFont(content::Id const & fontName,
                        content::Manager & contentManager)
    {
        auto lease = contentManager.borrow(fontName);
        assert(lease);
        setFont(lease->as<models::Font>());
    }

    void Label::setFont(models::Font const & fontData)
    {
        _fontRegular = _fonts.get(fontData._regular);
        _fontBold = _fonts.get(fontData._bold);
        _fontItalic = _fonts.get(fontData._italic);

        assert(_fontRegular);
        assert(_fontBold);
        assert(_fontItalic);

        _glyphSize = _fontRegular->glyphSize();

        if (_glyphSize != _fontBold->glyphSize())
        {
            MINIRE_THROW("fonts differs in size: {}, {}",
                         fontData._regular, fontData._bold);
        }
        if (_glyphSize != _fontItalic->glyphSize())
        {
            MINIRE_THROW("fonts differs in size: {}, {}",
                         fontData._regular, fontData._italic);
        }

        assert(fontData._glyphWidth == _glyphSize.x);
        assert(fontData._glyphHeight == _glyphSize.y);
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

    void Label::set(size_t row, size_t col,
                    text::FormattedString const & string)
    {
        size_t const rows = _symbols.rows();
        size_t const cols = _symbols.cols();

        for(auto const & i : string)
        {
            if (col >= cols)
            {
                col = 0;
                ++row;
            }

            switch(i.second)
            {
                case L'\n':
                    col = 0;
                    ++row;
                    break;

                case L'\r':
                    col = 0;
                    break;
            }

            if (row >= rows)
            {
                MINIRE_ERROR("too small label: {} >= {}", row, rows);
                return;
            }

            at(row, col).set(i.first, i.second);
            ++col;
        }
    }

    void Label::setPosition(float x, float y)
    {
        _position = pixelFix(glm::vec2(x, y));
    }

    void Label::revalidate() const
    {
        assert(_buffer);
        assert(_fontRegular);
        assert(_fontBold);
        assert(_fontItalic);

        _buffer->update(_symbols,
                        _dirty,
                        *_fontRegular,
                        *_fontBold,
                        *_fontItalic,
                        _cursor,
                        _glyphSize);

        _dirty.clear();
        _invalidated = false;
    }

    void Label::setCursor(size_t column, size_t row)
    {
        text::Symbol & symbol = at(row, column);
        if (symbol.blank())
        {
            symbol.set(L'\0');
        }

        if (_cursor._shown)
        {
            _dirty.emplace_back(_cursor._row, _cursor._column);
        }

        // dont need, at() will invalidate and mark as dirty:
        //_dirty.emplace_back(row, column); 
        //_invalidated = true;

        _cursor._shown = true;
        _cursor._column = column;
        _cursor._row = row;
    }

    void Label::unsetCursor()
    {
        if (_cursor._shown)
        {
            _dirty.emplace_back(_cursor._row, _cursor._column);
            _cursor._shown = false;
            _invalidated = true;
        }
    }
}
