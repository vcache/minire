#include <rasterizer/billboards.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/models/billboard.hpp>
#include <minire/models/font-face.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <rasterizer/font.hpp>
#include <rasterizer/fonts.hpp>
#include <rasterizer/labels/vertex-buffer.hpp>
#include <rasterizer/sprites/vertex-buffer.hpp>
#include <rasterizer/textures.hpp>
#include <scene.hpp>
#include <utils/overloaded.hpp>

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr

#include <array>
#include <cassert>
#include <cmath>

// TODO: adjust z-coordinate to handle z-fighting

namespace minire::rasterizer
{
    class Billboards::Program
    {
    public:
        virtual ~Program() = default;
        virtual void use(glm::mat4 const & transform,
                         scene::Viewpoint const &,
                         glm::mat4 const & viewProj) const = 0;
    };

    namespace
    {
        class WorldPlacedSprite
            : public Billboards::Program
        {
            static constexpr auto kVertShader =
            R"(
                #version 330 core

                layout(location = 0) in vec2 bznkPos;
                layout(location = 1) in vec2 bznkUv;
                layout(location = 2) in vec2 bznkRep;
                layout(location = 3) in vec2 bznkDims;

                uniform vec3 bznkCenter;
                uniform mat4 bznkView;
                uniform mat4 bznkProjection;

                out vec2 bznkFragUv;
                flat out vec2 bznkFragRep;
                flat out vec2 bznkFragDims;

                void main()
                {
                    // TODO: calc on CPU just once
                    vec3 cameraRight = vec3(bznkView[0][0], bznkView[1][0], bznkView[2][0]);
                    vec3 cameraUp    = vec3(bznkView[0][1], bznkView[1][1], bznkView[2][1]);

                    vec3 position = bznkCenter + cameraRight * bznkPos.x
                                               + cameraUp * bznkPos.y;

                    // TODO: calced from CPU
                    gl_Position = bznkProjection * bznkView * vec4(position, 1.0);

                    bznkFragUv = bznkUv;
                    bznkFragRep = bznkRep;
                    bznkFragDims = bznkDims;
                }
            )";

            static constexpr auto kFragShader =
            R"(
                #version 330 core

                in vec2 bznkFragUv;
                flat in vec2 bznkFragRep;
                flat in vec2 bznkFragDims;

                uniform sampler2D bznkTexture;

                out vec4 bznkOutCol;

                vec2 sawtooth(vec2 t)
                {
                    return fract(t);
                }

                void main()
                {
                    vec2 offset = bznkFragRep + bznkFragDims * sawtooth(bznkFragUv);
                    bznkOutCol = texture(bznkTexture, offset);
                }
            )";

        public:
            WorldPlacedSprite()
                : _program({std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, kVertShader),
                            std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader)})
                , _bznkCenter(_program.getUniformLocation("bznkCenter"))
                , _bznkView(_program.getUniformLocation("bznkView"))
                , _bznkProjection(_program.getUniformLocation("bznkProjection"))
                , _bznkTexture(_program.getUniformLocation("bznkTexture"))
            {
                // TOOD: _program.use(); ??
            }

            void use(glm::mat4 const & transform,
                     scene::Viewpoint const & viewpoint,
                     glm::mat4 const &) const override
            {
                _program.use();

                glm::vec3 const translate = transform[3];
                _program.setUniform(_bznkCenter, translate);
                _program.setUniform(_bznkView, viewpoint.view());
                _program.setUniform(_bznkProjection, viewpoint.projection());
                _program.setUniform(_bznkTexture, 0);
            }

        private:
            opengl::Program _program;
            GLint           _bznkCenter = 0;
            GLint           _bznkView = 0;
            GLint           _bznkProjection = 0;
            GLint           _bznkTexture = 0;
        };

        class ScreenPlacedSprite
            : public Billboards::Program
        {
            static constexpr auto kVertShader =
            R"(
                #version 330 core

                layout(location = 0) in vec2 bznkPos;
                layout(location = 1) in vec2 bznkUv;
                layout(location = 2) in vec2 bznkRep;
                layout(location = 3) in vec2 bznkDims;

                uniform vec2 bznkScreenFactor;
                uniform vec3 bznkCenter;
                uniform mat4 bznkViewProj;

                out vec2 bznkFragUv;
                flat out vec2 bznkFragRep;
                flat out vec2 bznkFragDims;

                void main()
                {
                    gl_Position = bznkViewProj * vec4(bznkCenter, 1.0f);
                    gl_Position /= gl_Position.w;
                    gl_Position.xy += bznkPos * bznkScreenFactor;

                    bznkFragUv = bznkUv;
                    bznkFragRep = bznkRep;
                    bznkFragDims = bznkDims;
                }
            )";

            static constexpr auto kFragShader =
            R"(
                #version 330 core

                in vec2 bznkFragUv;
                flat in vec2 bznkFragRep;
                flat in vec2 bznkFragDims;

                uniform sampler2D bznkTexture;

                out vec4 bznkOutCol;

                vec2 sawtooth(vec2 t)
                {
                    return fract(t);
                }

                void main()
                {
                    ivec2 offset = ivec2(floor(bznkFragRep + bznkFragDims * sawtooth(bznkFragUv)));
                    bznkOutCol = texelFetch(bznkTexture, offset, 0);
                }
            )";

        public:
            ScreenPlacedSprite()
                : _program({std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, kVertShader),
                            std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader)})
                , _bznkScreenFactor(_program.getUniformLocation("bznkScreenFactor"))
                , _bznkCenter(_program.getUniformLocation("bznkCenter"))
                , _bznkViewProj(_program.getUniformLocation("bznkViewProj"))
                , _bznkTexture(_program.getUniformLocation("bznkTexture"))
            {}

            void use(glm::mat4 const & transform,
                     scene::Viewpoint const & viewpoint,
                     glm::mat4 const & viewProj) const override
            {
                _program.use();

                glm::vec2 const screenFactor(2.0f / viewpoint.width(),
                                             2.0f / viewpoint.height());
                glm::vec3 const translate = transform[3];

                _program.setUniform(_bznkScreenFactor, screenFactor);
                _program.setUniform(_bznkCenter, translate);
                _program.setUniform(_bznkViewProj, viewProj);
                _program.setUniform(_bznkTexture, 0);
            }

        private:
            opengl::Program _program;
            GLint           _bznkScreenFactor = 0;
            GLint           _bznkCenter = 0;
            GLint           _bznkViewProj = 0;
            GLint           _bznkTexture = 0;
        };

        class WorldPlacedLabel
            : public Billboards::Program
        {
            static constexpr auto kVertShader =
            R"(
                #version 330 core

                layout(location = 0) in vec2 bznkPos;
                layout(location = 1) in vec2 bznkUv;
                layout(location = 2) in vec4 bznkFgColor;
                layout(location = 3) in vec4 bznkBgColor;
                layout(location = 4) in uint bznkFont;

                uniform vec3 bznkCenter;
                uniform mat4 bznkView;
                uniform mat4 bznkProjection;

                out vec2 bznkFragPos;
                out vec2 bznkFragUv;
                flat out vec4 bznkFragFgColor;
                flat out vec4 bznkFragBgColor;
                flat out uint bznkFragFont;

                void main()
                {
                    // TODO: calc on CPU just once
                    vec3 cameraRight = vec3(bznkView[0][0], bznkView[1][0], bznkView[2][0]);
                    vec3 cameraUp    = vec3(bznkView[0][1], bznkView[1][1], bznkView[2][1]);

                    vec3 position = bznkCenter + cameraRight * bznkPos.x
                                               + cameraUp * bznkPos.y;

                    // TODO: calced from CPU
                    gl_Position = bznkProjection * bznkView * vec4(position, 1.0);

                    bznkFragUv = bznkUv;
                    bznkFragFgColor = bznkFgColor;
                    bznkFragBgColor = bznkBgColor;
                    bznkFragFont = bznkFont;
                }
            )";

            static constexpr auto kFragShader =
            R"(
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
                    // TODO: should use texture() instead texel() (but have to normalize UVs)
                    ivec2 offset = ivec2(bznkFragUv);
                    float fgFactor = texelFetch(bznkFonts[bznkFragFont], offset, 0).r;

                    //float fgFactor = texture(bznkFonts[bznkFragFont], bznkFragUv).r;
                    float bgFactor = 1.0 - fgFactor;
                    bznkOutColor = bznkFragBgColor * bgFactor
                                 + bznkFragFgColor * fgFactor;
                }
            )";

        public:
            WorldPlacedLabel()
                : _program({std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, kVertShader),
                            std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader)})
                , _bznkCenter(_program.getUniformLocation("bznkCenter"))
                , _bznkView(_program.getUniformLocation("bznkView"))
                , _bznkProjection(_program.getUniformLocation("bznkProjection"))
                , _bznkFonts(_program.getUniformLocation("bznkFonts"))
            {}

            void use(glm::mat4 const & transform,
                     scene::Viewpoint const & viewpoint,
                     glm::mat4 const &) const override
            {
                static const std::array<GLint, 3> kTextureUnits{0, 1, 2};

                _program.use();

                glm::vec3 const translate = transform[3];
                _program.setUniform(_bznkCenter, translate);
                _program.setUniform(_bznkView, viewpoint.view());
                _program.setUniform(_bznkProjection, viewpoint.projection());
                _program.setUniform(_bznkFonts, kTextureUnits);
            }

        private:
            opengl::Program _program;
            GLint           _bznkCenter = 0;
            GLint           _bznkView = 0;
            GLint           _bznkProjection = 0;
            GLint           _bznkFonts = 0;
        };

        class ScreenPlacedLabel
            : public Billboards::Program
        {
            static constexpr auto kVertShader =
            R"(
                #version 330 core

                layout(location = 0) in vec2 bznkPos;
                layout(location = 1) in vec2 bznkUv;
                layout(location = 2) in vec4 bznkFgColor;
                layout(location = 3) in vec4 bznkBgColor;
                layout(location = 4) in uint bznkFont;

                uniform vec2 bznkScreenFactor;
                uniform vec3 bznkCenter;
                uniform mat4 bznkViewProj;

                out vec2 bznkFragPos;
                out vec2 bznkFragUv;
                flat out vec4 bznkFragFgColor;
                flat out vec4 bznkFragBgColor;
                flat out uint bznkFragFont;

                void main()
                {
                    gl_Position = bznkViewProj * vec4(bznkCenter, 1.0f);
                    gl_Position /= gl_Position.w;
                    gl_Position.xy += bznkPos * bznkScreenFactor;
                    gl_Position.xy -= mod(gl_Position.xy, bznkScreenFactor);

                    bznkFragUv = bznkUv;
                    bznkFragFgColor = bznkFgColor;
                    bznkFragBgColor = bznkBgColor;
                    bznkFragFont = bznkFont;
                }
            )";

            static constexpr auto kFragShader =
            R"(
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

        public:
            ScreenPlacedLabel()
                : _program({std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, kVertShader),
                            std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader)})
                , _bznkScreenFactor(_program.getUniformLocation("bznkScreenFactor"))
                , _bznkCenter(_program.getUniformLocation("bznkCenter"))
                , _bznkViewProj(_program.getUniformLocation("bznkViewProj"))
                , _bznkFonts(_program.getUniformLocation("bznkFonts"))
            {}

            void use(glm::mat4 const & transform,
                     scene::Viewpoint const & viewpoint,
                     glm::mat4 const & viewProj) const override
            {

                static const std::array<GLint, 3> kTextureUnits{0, 1, 2};

                _program.use();

                glm::vec2 const screenFactor(2.0f / viewpoint.width(),
                                             2.0f / viewpoint.height());
                glm::vec3 const translate = transform[3];

                _program.setUniform(_bznkScreenFactor, screenFactor);
                _program.setUniform(_bznkCenter, translate);
                _program.setUniform(_bznkViewProj, viewProj);
                _program.setUniform(_bznkFonts, kTextureUnits);
            }

        private:
            opengl::Program _program;
            GLint           _bznkScreenFactor = 0;
            GLint           _bznkCenter = 0;
            GLint           _bznkViewProj = 0;
            GLint           _bznkFonts = 0;
        };
    }

    // Billboard //

    class Billboard
    {
    public:
        virtual ~Billboard() = default;
        virtual void draw(glm::mat4 const & transform,
                          scene::Viewpoint const & viewpoint,
                          glm::mat4 const & viewProj) const = 0;
    };

    namespace
    {
        class SpriteBillboard
            : public rasterizer::Billboard
        {
        public:
            explicit SpriteBillboard(Textures::Texture::Sptr const & texture,
                                     std::vector<sprites::Vertex> const & vertices,
                                     Billboards::ProgramSptr const & program)
                : _texture(texture)
                , _vertexBuffer(vertices)
                , _program(program)
            {
                assert(_program);
            }

            void draw(glm::mat4 const & transform,
                      scene::Viewpoint const & viewpoint,
                      glm::mat4 const & viewProj) const override
            {
                assert(_program);
                _program->use(transform, viewpoint, viewProj);

                MINIRE_GL(glActiveTexture, GL_TEXTURE0);
                assert(_texture);
                _texture->bind();

                _vertexBuffer.draw();
            }

        private:
            Textures::Texture::Sptr _texture;
            sprites::VertexBuffer   _vertexBuffer;
            Billboards::ProgramSptr _program;
        };

        class LabelBillboard
            : public rasterizer::Billboard
        {
        public:
            using FontSptr = std::shared_ptr<Font const>;

            explicit LabelBillboard(FontSptr fontRegular,
                                    FontSptr fontBold,
                                    FontSptr fontItalic,
                                    std::vector<labels::Vertex> const & vertices,
                                    Billboards::ProgramSptr program)
                : _fontRegular(fontRegular)
                , _fontBold(fontBold)
                , _fontItalic(fontItalic)
                , _vertexBuffer(vertices)
                , _program(program)
            {}

            void draw(glm::mat4 const & transform,
                      scene::Viewpoint const & viewpoint,
                      glm::mat4 const & viewProj) const override
            {

                assert(_program);
                _program->use(transform, viewpoint, viewProj);

                MINIRE_GL(glActiveTexture, GL_TEXTURE0);
                assert(_fontRegular);
                _fontRegular->bind();

                MINIRE_GL(glActiveTexture, GL_TEXTURE1);
                assert(_fontBold);
                _fontBold->bind();

                MINIRE_GL(glActiveTexture, GL_TEXTURE2);
                assert(_fontItalic);
                _fontItalic->bind();

                _vertexBuffer.draw();
            }

        private:
            FontSptr                _fontRegular;
            FontSptr                _fontBold;
            FontSptr                _fontItalic;
            labels::VertexBuffer    _vertexBuffer;
            Billboards::ProgramSptr _program;
        };
    }

    namespace
    {
        template<typename Vertex>
        glm::vec2 measureVertexArray(std::vector<Vertex> const & vertices)
        {
            if(vertices.empty())
                return glm::vec2(0);

            glm::vec2 contentMax(vertices.front()._pos),
                      contentMin(vertices.front()._pos);

            for(Vertex const & vertex : vertices)
            {
                contentMin = glm::min(contentMin, vertex._pos);
                contentMax = glm::max(contentMax, vertex._pos);
            }
            return contentMax - contentMin;
        }

        template<typename Vertex>
        void flipHorizontal(glm::vec2 const & realContentSize,
                            std::vector<Vertex> & vertices)
        {
            for(Vertex & vertex : vertices)
            {
                // NOTE: assuming that GL_CULL_FACE will be disabled
                vertex._pos.y = realContentSize.y - vertex._pos.y;
            }
        }

        template<typename Vertex>
        void adjustWorldPosition(glm::vec2 const & realContentSize,
                                 models::Billboard::World const & world,
                                 std::vector<Vertex> & vertices)
        {
            float const ratio = realContentSize.y / realContentSize.x;
            glm::vec2 const pixToWorld(1.0f / realContentSize.x,
                                       1.0f / realContentSize.y * ratio);
            glm::vec2 const alignment = world._scale * .5f;
            for(Vertex & vertex : vertices)
            {
                vertex._pos *= pixToWorld;
                vertex._pos *= world._scale;
                vertex._pos -= alignment;
                vertex._pos += world._translate;
            }
        }

        glm::vec2 pixelFix(glm::vec2 const & in)
        {
            return glm::vec2(
                std::floor(in.x) + .5f,
                std::floor(in.y) + .5f
            );
        }
    }

    // Billboards //

    Billboards::Billboards(content::Manager & contentManager,
                           Fonts const & fonts,
                           Textures const & textures)
        : _contentManager(contentManager)
        , _fonts(fonts)
        , _textures(textures)
        , _worldPlacedSprite(std::make_shared<WorldPlacedSprite>())
        , _screenPlacedSprite(std::make_shared<ScreenPlacedSprite>())
        , _worldPlacedLabel(std::make_shared<WorldPlacedLabel>())
        , _screenPlacedLabel(std::make_shared<ScreenPlacedLabel>())
    {}

    std::shared_ptr<Billboard>
    Billboards::create(models::Billboard const & billboard) const
    {
        return std::visit([this, billboard](auto const & content)
                          { return create(billboard, content); },
                          billboard._content);
    }

    std::shared_ptr<Billboard>
    Billboards::create(models::Billboard const & billboard,
                       models::Billboard::Sprite const & sprite) const
    {
        auto textureSptr = _textures.get(sprite._texture, {}, false /* no mipmap */);
        MINIRE_INVARIANT(textureSptr, "no texture found for a billboard: {}",
                         sprite._texture);
        glm::vec2 const textureSize(textureSptr->width(),
                                    textureSptr->height());

        auto vertices = sprites::buildMesh(sprite._source,
                                           glm::vec2(0, 0),
                                           sprite._contentSize,
                                           textureSize);
        glm::vec2 const realContentSize = measureVertexArray(vertices);
        flipHorizontal(realContentSize, vertices);

        return std::visit(utils::Overloaded
        {
            [&] (models::Billboard::World const & world)
            {
                adjustWorldPosition(realContentSize, world, vertices);
                for(sprites::Vertex & vertex : vertices)
                {
                    vertex._rep /= textureSize;
                    vertex._dims /= textureSize;
                }
                return std::make_shared<SpriteBillboard>(textureSptr, vertices, _worldPlacedSprite);
            },

            [&](models::Billboard::Screen const & screen)
            {
                glm::vec2 const alignment = realContentSize * .5f;
                for(sprites::Vertex & vertex : vertices)
                {
                    vertex._pos -= alignment;
                    vertex._pos += screen._screenOffset;
                }
                return std::make_shared<SpriteBillboard>(textureSptr, vertices, _screenPlacedSprite);
            }
        }, billboard._placement);
    }

    std::shared_ptr<Billboard>
    Billboards::create(models::Billboard const & billboard,
                       models::Billboard::Label const & label) const
    {
        // TODO: code duplicated w/ label.cpp
        auto lease = _contentManager.borrow(label._fontFace);
        assert(lease);
        models::FontFace const & fontFace = lease->as<models::FontFace>();

        auto fontRegular = _fonts.get(fontFace._regular);
        auto fontBold = _fonts.get(fontFace._bold);
        auto fontItalic = _fonts.get(fontFace._italic);

        assert(fontRegular);
        assert(fontBold);
        assert(fontItalic);

        glm::vec2 const glyphSize = fontRegular->glyphSize();
        MINIRE_INVARIANT(glyphSize == fontBold->glyphSize(),
                         "only monospaced fonts are supported, "
                         "but fonts differ in size: {}, {}",
                         fontFace._regular, fontFace._bold);
        MINIRE_INVARIANT(glyphSize == fontItalic->glyphSize(),
                         "only monospaced fonts are supported, "
                         "but fonts differ in size: {}, {}",
                         fontFace._regular, fontFace._italic);
        MINIRE_INVARIANT(fontFace._glyphWidth == glyphSize.x &&
                         fontFace._glyphHeight == glyphSize.y,
                         "glyph sizes differ from models: {}x{} != {}x{}",
                         fontFace._glyphWidth, fontFace._glyphHeight,
                         glyphSize.x, glyphSize.y);

        // TODO: don't re-create VAO/VBO, but update the existing ones
        std::vector<labels::Vertex> vertices = labels::buildMesh(
            label._text, *fontRegular, *fontBold, *fontItalic, std::nullopt);
        glm::vec2 const realContentSize = measureVertexArray(vertices);
        flipHorizontal(realContentSize, vertices);

        return std::visit(utils::Overloaded
        {
            [&] (models::Billboard::World const & world)
            {
                adjustWorldPosition(realContentSize, world, vertices);
                return std::make_shared<LabelBillboard>(
                    fontRegular, fontBold, fontItalic, vertices, _worldPlacedLabel);
            },

            [&](models::Billboard::Screen const & screen)
            {
                glm::vec2 const alignment = realContentSize * .5f;
                for(labels::Vertex & vertex : vertices)
                {
                    vertex._pos -= alignment;
                    vertex._pos += screen._screenOffset;
                    vertex._pos = pixelFix(vertex._pos);
                }
                return std::make_shared<LabelBillboard>(
                    fontRegular, fontBold, fontItalic, vertices, _screenPlacedLabel);
            }

        }, billboard._placement);
    }

    void Billboards::draw(Scene const & scene) const
    {
        scene::Viewpoint const & viewpoint = scene.viewpoint();
        assert(viewpoint.hasCamera());

        auto const viewProj = viewpoint.projection() * viewpoint.view();

        // NOTE: assuming that billboards from the same node are already sorted
        //       (i.e. has correct relative order)
        scene.cullBillboards(
            [&viewpoint, &viewProj](rasterizer::Billboard const & billboard,
                                    glm::mat4 const & transform)
            {
                billboard.draw(transform, viewpoint, viewProj);
            });
    }
}