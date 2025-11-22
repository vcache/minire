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
#include <rasterizer/labels/vertex-buffer.hpp>

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

    Label::Label(Fonts const & fonts,
                 text::FormattedString const & text,
                 size_t z, bool visible)
        : Drawable(z)
        , _fonts(fonts)
        , _text(text)
        , _position(0.0)
        , _program(Program::instance())
        , _vertexBuffer()
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

        assert(_vertexBuffer);
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

        _vertexBuffer->draw();
    }

    void Label::setPosition(glm::vec2 position)
    {
        _position = pixelFix(position);
    }

    void Label::revalidate() const
    {
        assert(_fontRegular);
        assert(_fontBold);
        assert(_fontItalic);

        // TODO: don't re-create VAO/VBO, but update the existing ones
        auto const & vertices = labels::buildMesh(
            _text, *_fontRegular, *_fontBold, *_fontItalic, _maxSize);
        _vertexBuffer = std::make_unique<labels::VertexBuffer>(vertices);

        _invalidated = false;
    }
}
