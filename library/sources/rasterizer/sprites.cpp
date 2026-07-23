#include <rasterizer/sprites.hpp>

#include <minire/errors.hpp>
#include <minire/utils/overloaded.hpp>
#include <minire/utils/rect.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <rasterizer/sprites/vertex-buffer.hpp>
#include <rasterizer/textures.hpp>

#include <glm/gtc/type_ptr.hpp> // for glm::value_ptr
#include <glm/mat4x4.hpp>

#include <cassert>
#include <variant>
#include <vector>

namespace minire::rasterizer
{
    // Sprites::Program //

    static const char * kVertShader = R"(
        #version 330 core

        layout(location = 0) in vec2 bznkPos;
        layout(location = 1) in vec2 bznkUv;
        layout(location = 2) in vec2 bznkRep;
        layout(location = 3) in vec2 bznkDims;
        uniform mat4 bznkProj;
        out vec2 bznkFragUv;
        flat out vec2 bznkFragRep;
        flat out vec2 bznkFragDims;

        void main()
        {
            gl_Position = bznkProj * vec4(bznkPos, 0.0, 1.0);
            bznkFragUv = bznkUv;
            bznkFragRep = bznkRep;
            bznkFragDims = bznkDims;
        }
    )";

    static const char * kFragShader = R"(
        #version 330 core

        in vec2 bznkFragUv;
        flat in vec2 bznkFragRep;
        flat in vec2 bznkFragDims;
        uniform sampler2D bznkTexture;
        uniform vec4 bznkClippingWindow;  // (left, top, right, bottom)
        out vec4 bznkOutColor;

        layout(origin_upper_left) in vec4 gl_FragCoord;

        vec2 sawtooth(vec2 t)
        {
            return fract(t);
        }

        void main()
        {
            if (bznkClippingWindow.x <= gl_FragCoord.x && gl_FragCoord.x <= bznkClippingWindow.z
             && bznkClippingWindow.y <= gl_FragCoord.y && gl_FragCoord.y <= bznkClippingWindow.w)
            {
                ivec2 offset = ivec2(floor(bznkFragRep + bznkFragDims * sawtooth(bznkFragUv)));

                bznkOutColor = texelFetch(bznkTexture, offset, 0);

                //bznkOutColor = vec4(fract(bznkFragUv.y), 0, 0, 1);
            }
            else
            {
                discard;
            }
        }
    )";

    class Sprites::Program
    {
    public:
        Program()
            : _program({
                std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, kVertShader),
                std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader)
            })
            , _projUniform(_program.getUniformLocation("bznkProj"))
            , _textureUniform(_program.getUniformLocation("bznkTexture"))
            , _clippingWindow(_program.getUniformLocation("bznkClippingWindow"))
        {}

        void use() const { _program.use(); }

        void setProjUniform(glm::mat4 const & m) const
        {
            MINIRE_GL(glUniformMatrix4fv, _projUniform, 1,
                      GL_FALSE, glm::value_ptr(m));
        }

        void setTextureUniform(GLint const v) const
        {
            MINIRE_GL(glUniform1i, _textureUniform, v);
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
        GLint           _projUniform;
        GLint           _textureUniform;
        GLint           _clippingWindow;
    };

    // Sprites::SpriteImpl //

    class Sprites::SpriteImpl final
        : public Sprite
        , public Drawable
    {
    public:
        SpriteImpl(std::string const & name,
                   models::Sprite model,
                   Textures const & textures,
                   Program const & program)
            : Sprite(name, std::move(model), kNoFlags)
            , _textures(textures)
            , _program(program)
            , _texture(fetchTexture())
            , _vertexBuffer(buildMesh())
        {}

    public:
        void draw(glm::mat4 const & projection) override
        {
            revalidate();

            _program.use();
            _program.setProjUniform(projection);
            _program.setTextureUniform(0);
            _program.setClippingWindow(model()._clippingWindow);

            MINIRE_GL(glActiveTexture, GL_TEXTURE0);
            assert(_texture);
            _texture->bind();

            _vertexBuffer.draw();
        }

    private:
        void revalidate(Mask mask = kAllFlags) override
        {
            if (invalidated())
            {
                bool textureResized = false;
                if (invalidatedAny(kTexture))
                {
                    assert(_texture);
                    float oldWidth = _texture->width();
                    float oldHeight = _texture->height();

                    _texture = fetchTexture();

                    assert(_texture);
                    textureResized = oldWidth != _texture->width()
                                  || oldHeight != _texture->height();
                }

                if (textureResized || invalidatedAny(kPatch | kPosition | kDimensions))
                {
                    _vertexBuffer.reload(buildMesh());
                }

                Object::revalidate(mask);
            }
        }

        Textures::Texture::Sptr fetchTexture() const
        {
            models::Sprite const & m = model();
            Textures::Texture::Sptr result = _textures.get(m._image._texture, {}, false /* no mipmap */);
            MINIRE_INVARIANT(result, "no texture found: {}", m._image._texture);
            return result;
        }

        std::vector<sprites::Vertex> buildMesh() const
        {
            assert(_texture);
            models::Sprite const & m = model();
            return sprites::buildMesh(m._image._patch, m._position, m._dimensions,
                                      glm::vec2(_texture->width(), _texture->height()));

        }

    private:
        Textures const &        _textures;
        Program const &         _program;
        Textures::Texture::Sptr _texture;
        sprites::VertexBuffer   _vertexBuffer;
    };

    // Sprites //

    Sprites::Sprites(Textures const & textures)
        : _textures(textures)
        , _program(std::make_unique<Program>())
    {}

    // because of std::unique_ptr<Program>
    Sprites::~Sprites() = default;

    Sprite::Sptr Sprites::make(std::string const & name,
                               models::Sprite model)
    {
        assert(_program);
        auto result = std::make_shared<SpriteImpl>(
            name, std::move(model), _textures, *_program);

        if (!name.empty())
        {
            auto [_, inserted] = _index.emplace(name, result);
            MINIRE_INVARIANT(inserted, "failed to create a make: \"{}\" (a duplicate?)", name);
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

    Sprite::Sptr Sprites::find(std::string const & name) const
    {
        if (auto it = _index.find(name); it != _index.cend())
        {
            if (Sprite::Sptr const & result = it->second;
                result && !result->detached())
            {
                assert(result->name() == name);
                return result;
            }
        }

        return {};
    }

    void Sprites::predraw(Drawable::PtrsList & out) const
    {
        // TODO: sort by visibility (skip invisibles)
        // TODO: sort by a texture
        auto it = _heap.begin();
        while (it != _heap.end())
        {
            SpriteImplSptr const & sprite = *it;
            assert(sprite);

            if (!sprite->detached())
            {
                if (sprite->visible())
                {
                    sprite->setEffectiveZOrder(sprite->zOrder());
                    out.emplace_back(sprite.get());
                }
                it++;
            }
            else
            {
                if (!sprite->name().empty())
                {
                    _index.erase(sprite->name());
                }
                it = _heap.erase(it);
            }
        }
    }
}
