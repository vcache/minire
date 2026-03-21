#include <rasterizer/labels.hpp>

#include <minire/content/asset.hpp>
#include <minire/content/id.hpp>
#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/label.hpp>
#include <minire/logging.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/text/symbol.hpp>
#include <minire/text/text-format.hpp>
#include <minire/utils/rect.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>
#include <rasterizer/drawable.hpp>
#include <rasterizer/font.hpp>
#include <rasterizer/fonts.hpp>
#include <rasterizer/labels/vertex-buffer.hpp>

#include <glm/common.hpp> // for glm::value_ptr
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include <cassert>
#include <cmath>
#include <limits>

namespace minire::rasterizer
{
    // Labels::Program //

    static const char * kVertShader = R"(
        #version 330 core

        layout(location = 0) in vec2 bznkPos;
        layout(location = 1) in vec2 bznkUv;
        layout(location = 2) in vec4 bznkFgColor;
        layout(location = 3) in vec4 bznkBgColor;
        layout(location = 4) in uint bznkFont;

        uniform mat4 bznkProj;
        uniform vec2 bznkPosition;

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

        layout(origin_upper_left) in vec4 gl_FragCoord;

        in vec2 bznkFragUv;
        flat in vec4 bznkFragFgColor;
        flat in vec4 bznkFragBgColor;
        flat in uint bznkFragFont;

        uniform sampler2D bznkFonts[3];
        uniform vec4 bznkClippingWindow;  // (left, top, right, bottom)

        out vec4 bznkOutColor;

        void main()
        {
            if (bznkClippingWindow.x <= gl_FragCoord.x && gl_FragCoord.x <= bznkClippingWindow.z
             && bznkClippingWindow.y <= gl_FragCoord.y && gl_FragCoord.y <= bznkClippingWindow.w)
            {
                ivec2 offset = ivec2(bznkFragUv);
                float fgFactor = texelFetch(bznkFonts[bznkFragFont], offset, 0).r;

                //float fgFactor = texture(bznkFonts[bznkFragFont], bznkFragUv).r;
                float bgFactor = 1.0 - fgFactor;
                bznkOutColor = bznkFragBgColor * bgFactor
                             + bznkFragFgColor * fgFactor;
            }
            else
            {
                discard;
            }
        }
    )";

    class Labels::Program
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
            , _clippingWindow(_program.getUniformLocation("bznkClippingWindow"))
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

        void setClippingWindow(utils::MaybeRect const & clippingWindow) const
        {
            glm::vec4 value(std::numeric_limits<float>::min(),
                            std::numeric_limits<float>::min(),
                            std::numeric_limits<float>::max(),
                            std::numeric_limits<float>::max());
            if (clippingWindow)
            {
                value = glm::vec4(clippingWindow->_left,  clippingWindow->_top,
                                  clippingWindow->_right, clippingWindow->_bottom);
            }
            _program.setUniform(_clippingWindow, value);
        }

    private:
        opengl::Program _program;
        GLint           _fontsUniform;
        GLint           _projUniform;
        GLint           _positionUniform;
        GLint           _clippingWindow;
    };

    // Labels::LabelImpl //

    class Labels::LabelImpl final
        : public Label
        , public Drawable
    {
    public:
        LabelImpl(std::string const & name,
                  models::Label model,
                  Fonts const & fonts,
                  Program const & program,
                  content::Manager & contentManager)
            : Label(name, std::move(model))
            , _fonts(fonts)
            , _program(program)
            , _contentManager(contentManager)
        {
            invalidate();
        }

    public:
        // TODO: consider to put projection into UBO
        void draw(glm::mat4 const & projection) override
        {
            static const std::array<GLint, 3> kTextureUnits{0, 1, 2};

            revalidate();

            _program.use();

            MINIRE_GL(glActiveTexture, GL_TEXTURE0);
            assert(_fontRegular);
            _fontRegular->bind();

            MINIRE_GL(glActiveTexture, GL_TEXTURE1);
            assert(_fontBold);
            _fontBold->bind();

            MINIRE_GL(glActiveTexture, GL_TEXTURE2);
            assert(_fontItalic);
            _fontItalic->bind();

            models::Label const & m = model();

            _program.setFontsUniform(kTextureUnits);
            _program.setProjUniform(projection);
            _program.setPositionUniform(pixelFix(m._position));
            _program.setClippingWindow(m._clippingWindow);

            assert(_vertexBuffer);
            _vertexBuffer->draw();
        }

    private:
        static glm::vec2 pixelFix(glm::vec2 const & in)
        {
            return glm::vec2(
                std::floor(in.x) + .5f,
                std::floor(in.y) + .5f
            );
        }

        void revalidate() override
        {
            if (invalidated())
            {
                models::Label const & m = model();

                if (invalidated(kFontFace))
                {
                    auto lease = _contentManager.borrow(m._fontFace);
                    assert(lease);
                    models::FontFace const & fontData = lease->as<models::FontFace>();

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
                }

                assert(_fontRegular);
                assert(_fontBold);
                assert(_fontItalic);

                if (invalidated(kFontFace | kText))
                {
                    // TODO: don't re-create VAO/VBO, but update the existing ones
                    auto const & vertices = labels::buildMesh(
                        m._text, *_fontRegular, *_fontBold, *_fontItalic);
                    _vertexBuffer = std::make_unique<labels::VertexBuffer>(vertices);
                }

                Object::revalidate();
            }
        }

    private:
        using FontPtr = std::shared_ptr<Font const>;
        using VertexBufferUptr = std::unique_ptr<labels::VertexBuffer>;

        Fonts const      & _fonts;
        Program const    & _program;
        content::Manager & _contentManager;

        FontPtr            _fontRegular;
        FontPtr            _fontBold;
        FontPtr            _fontItalic;
        VertexBufferUptr   _vertexBuffer;
    };

    // Labels //

    Labels::Labels(Fonts const & fonts,
                   content::Manager & contentManager)
        : _fonts(fonts)
        , _contentManager(contentManager)
        , _program(std::make_unique<Program>())
    {}

    // because of std::unique_ptr<Program>
    Labels::~Labels() = default;

    Label::Sptr Labels::make(std::string const & name,
                             models::Label model)
    {
        assert(_program);
        auto result = std::make_shared<LabelImpl>(name,
                                                  std::move(model),
                                                  _fonts,
                                                  *_program,
                                                  _contentManager);

        if (!name.empty())
        {
            auto [_, inserted] = _index.emplace(name, result);
            MINIRE_INVARIANT(inserted, "failed to make a label: \"{}\" (a duplicate?)", name);
        }

        try
        {
            _heap.emplace_back(result);
            return result;
        }
        catch(...)
        {
            _index.erase(name);
            throw;
        }
    }

    Label::Sptr Labels::find(std::string const & name) const
    {
        if (auto it = _index.find(name); it != _index.cend())
        {
            if (LabelImpl::Sptr const & result = it->second;
                result && !result->detached())
            {
                assert(result->name() == name);
                return result;
            }
        }

        return {};
    }

    void Labels::predraw(Drawable::PtrsList & out) const
    {
        // TODO: sort by visibility (skip invisibles)
        // TODO: sort by a fontFace
        auto it = _heap.begin();
        while (it != _heap.end())
        {
            LabelImplSptr const & label = *it;
            assert(label);

            if (!label->detached())
            {
                if (label->visible())
                {
                    label->setEffectiveZOrder(label->zOrder());
                    out.emplace_back(label.get());
                }
                it++;
            }
            else
            {
                if (!label->name().empty())
                {
                    _index.erase(label->name());
                }
                it = _heap.erase(it);
            }
        }
    }
}
