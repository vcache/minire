#include <rasterizer/sprites.hpp>

#include <minire/errors.hpp>
#include <minire/utils/rect.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <rasterizer/sprites/vertex-buffer.hpp>
#include <rasterizer/textures.hpp>
#include <utils/overloaded.hpp>

#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr
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

    // Sprites::Sprite //

    class Sprites::Sprite : public Drawable
    {
    public:
        Sprite(Textures::Texture::Sptr texture,
               utils::Patch patch,
               glm::vec2 const & position,
               glm::vec2 const & dimensions,
               utils::MaybeRect const & clippingWindow,
               bool visible, size_t z,
               Program const & program)
            : Drawable(z)
            , _texture(texture)
            , _patch(patch)
            , _position(position)
            , _dimensions(dimensions)
            , _clippingWindow(clippingWindow)
            , _visible(visible)
            , _program(program)
            , _vertexBuffer(sprites::buildMesh(_patch, _position, _dimensions,
                                               glm::vec2(_texture->width(), _texture->height())))
            , _invalidated(false)
        {
            MINIRE_INVARIANT(_texture, "sprite created w/o a texture");
        }

    public:
        bool visible() const { return _visible; }

        void setPosition(glm::vec2 const & p)
        {
            _position = p;
            _invalidated = true;
        }

        void setDimensions(glm::vec2 const & d)
        {
            if (std::holds_alternative<utils::Rect>(_patch))
            {
                MINIRE_THROW("should not set dimensions for a sprite!");
            }

            _dimensions = d;
            _invalidated = true;
        }

        void setClippingWindow(utils::MaybeRect const & clippingWindow)
        {
            _clippingWindow = clippingWindow;
        }

        void setVisible(bool visible)
        {
            _visible = visible;
        }

        void draw(glm::mat4 const & projection) const override
        {
            revalidate();

            _program.use();
            _program.setProjUniform(projection);
            _program.setTextureUniform(0);
            _program.setClippingWindow(_clippingWindow);

            MINIRE_GL(glActiveTexture, GL_TEXTURE0);
            _texture->bind();

            _vertexBuffer.draw();
        }

    private:
        void revalidate() const
        {
            if (_invalidated)
            {
                assert(_texture);
                auto vertices = sprites::buildMesh(_patch, _position, _dimensions,
                                                   glm::vec2(_texture->width(), _texture->height()));
                _vertexBuffer.reload(vertices);
                _invalidated = false;
            }
        }

    private:
        Textures::Texture::Sptr       _texture;
        utils::Patch                  _patch;
        glm::vec2                     _position;
        glm::vec2                     _dimensions;
        utils::MaybeRect              _clippingWindow;
        bool                          _visible;
        Program const &               _program;

        mutable sprites::VertexBuffer _vertexBuffer;
        mutable bool                  _invalidated;
    };

    // Sprites //

    Sprites::Sprites(Textures const & textures)
        : _textures(textures)
        , _program(std::make_unique<Program>())
    {}

    Sprites::~Sprites() = default;

    void Sprites::create(std::string const & id,
                         content::Id const & texture,
                         utils::Patch const & patch,
                         glm::vec2 const & position,
                         glm::vec2 const & dimensions,
                         utils::MaybeRect const & clippingWindow,
                         bool const visible,
                         size_t const zOrder)
    {
        auto textureSptr = _textures.get(texture, {}, false /* no mipmap */);
        MINIRE_INVARIANT(textureSptr, "no texture found for \"{}\": {}", id, texture);

        auto sprite = std::make_unique<Sprite>(
            textureSptr, patch, position, dimensions,
            clippingWindow, visible, zOrder, *_program);

        auto [_, inserted] = _store.emplace(id, std::move(sprite));
        MINIRE_INVARIANT(inserted, "sprite already exists: \"{}\"", id);
    }

    void Sprites::move(std::string const & id,
                       glm::vec2 const & position,
                       utils::MaybeRect const & clippingWindow)
    {
        Sprite & sprite = find(id);
        sprite.setPosition(position);
        sprite.setClippingWindow(clippingWindow);
    }

    void Sprites::resize(std::string const & id,
                         glm::vec2 const & dimensions,
                         utils::MaybeRect const & clippingWindow)
    {
        Sprite & sprite = find(id);
        sprite.setDimensions(dimensions);
        sprite.setClippingWindow(clippingWindow);
    }

    void Sprites::setArea(std::string const & id,
                          glm::vec2 const & position,
                          glm::vec2 const & dimensions,
                          utils::MaybeRect const & clippingWindow)
    {
        Sprite & sprite = find(id);
        sprite.setPosition(position);
        sprite.setDimensions(dimensions);
        sprite.setClippingWindow(clippingWindow);
    }

    void Sprites::setClippingWindow(std::string const & id,
                                    utils::MaybeRect const & clippingWindow)
    {
        Sprite & sprite = find(id);
        sprite.setClippingWindow(clippingWindow);
    }

    void Sprites::visible(std::string const & id,
                          bool visible)
    {
        find(id).setVisible(visible);
    }

    void Sprites::setZOrder(std::string const & id,
                            size_t zOrder)
    {
        find(id).setZOrder(zOrder);
    }

    void Sprites::remove(std::string const & id)
    {
        _store.erase(id);
    }

    Sprites::Sprite & Sprites::find(std::string const & id) const
    {
        auto it = _store.find(id);
        MINIRE_INVARIANT(it != _store.cend(), "no such sprite: \"{}\"", id);
        return *it->second;
    }

    void Sprites::predraw(Drawable::PtrsList & out) const
    {
        // TODO: sort by visibility
        // TODO: sort by texture
        for(auto const & sprite : _store)
        {
            if (sprite.second->visible())
            {
                out.push_back(sprite.second.get());
            }
        }
    }
}
