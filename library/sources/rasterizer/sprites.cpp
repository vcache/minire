#include <rasterizer/sprites.hpp>

#include <minire/errors.hpp>
#include <minire/utils/rect.hpp>

#include <rasterizer/textures.hpp>
#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>
#include <utils/overloaded.hpp>

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr

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
        out vec4 bznkOutColor;

        vec2 sawtooth(vec2 t)
        {
            return fract(t);
        }

        void main()
        {
            ivec2 offset = ivec2(floor(bznkFragRep + bznkFragDims * sawtooth(bznkFragUv)));
            
            bznkOutColor = texelFetch(bznkTexture, offset, 0);

            //bznkOutColor = vec4(fract(bznkFragUv.y), 0, 0, 1);
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

    private:
        opengl::Program _program;
        GLint           _projUniform;
        GLint           _textureUniform;
    };

    // TileInfo //

    namespace
    {
        // NOTE: Clients must align data elements consistent with the 
        //       requirements of the client platform, with an additional
        //       base-level requirement that an offset within a buffer to
        //       a datum comprising N be a multiple of N.        
        struct Vertex
        {
            alignas(2) glm::vec2 _pos;
            alignas(2) glm::vec2 _uv;
            alignas(2) glm::vec2 _rep;
            alignas(2) glm::vec2 _dims;
        };

        using TileInfo = std::variant<utils::Rect, utils::NinePatch>;

        size_t getTileInfoSize(TileInfo const & tileInfo)
        {
            return std::visit(utils::Overloaded
            {
                [](utils::Rect const &)      -> size_t { return 6; },
                [](utils::NinePatch const &) -> size_t { return 6 * 9; },
            }, tileInfo);
        }

        class TileInfoVisitor
        {
        public:
            TileInfoVisitor(Textures::Texture const & texture,
                            glm::vec2 const & position,
                            glm::vec2 const & dimensions,
                            std::vector<Vertex> & vertices)
                : _texture(texture)
                , _position(position)
                , _dimensions(dimensions)
                , _vertices(vertices)
            {}

        public:
            void operator()(utils::Rect const & tile)
            {
                static const glm::vec2 kDp(0, 0);
                makeQuad(kDp, tile._left, tile._top,
                              tile._right, tile._bottom,
                              0, _vertices);
            }

            // TODO: parts are overlapping at +- 1 pixel
            void operator()(utils::NinePatch const & tile)
            {
                static const glm::vec2 kDp(0, 0);

                size_t offset = 0;
                
                // corners (boundary - out) //

                glm::vec2 corners(_dimensions.x - (tile._boundary._right - tile._out._right + 1.0f),
                                  _dimensions.y - (tile._out._top - tile._boundary._top + 1.0f));

                // III
                offset += makeQuad(kDp,
                    tile._boundary._left,
                    tile._out._bottom,
                    tile._out._left,
                    tile._boundary._bottom,
                    offset, _vertices);

                // IV
                offset += makeQuad(glm::vec2(corners.x, 0),
                    tile._out._right,
                    tile._out._bottom,
                    tile._boundary._right,
                    tile._boundary._bottom,
                    offset, _vertices);

                // I
                offset += makeQuad(glm::vec2(0, corners.y),
                    tile._boundary._left,
                    tile._boundary._top,
                    tile._out._left,
                    tile._out._top,
                    offset, _vertices);

                // II
                offset += makeQuad(corners,
                    tile._out._right,
                    tile._boundary._top,
                    tile._boundary._right,
                    tile._out._top,
                    offset, _vertices);

                // borders (out - in) //

                // bottom
                offset += makeQuad(
                    glm::vec2(tile._out._left - tile._boundary._left + 1.0f, 0),

                    _dimensions.x - (tile._out._left - tile._boundary._left + 1.0f)
                                  - (tile._boundary._right - tile._out._right + 1.0f),
                    tile._out._top - tile._boundary._top + 1.0f,

                    tile._in._left, tile._out._bottom,
                    tile._in._right, tile._boundary._bottom,
                    offset, _vertices);

                // left
                offset += makeQuad(
                    glm::vec2(0, tile._out._top - tile._boundary._top + 1.0f),

                    tile._out._left - tile._boundary._left + 1.0f,
                    _dimensions.y - (tile._out._top - tile._boundary._top + 1.0f)
                                  - (tile._boundary._bottom - tile._out._bottom + 1.0f),

                    tile._boundary._left, tile._in._top,
                    tile._out._right, tile._in._bottom,
                    offset, _vertices);

                // right
                offset += makeQuad(
                    glm::vec2(_dimensions.x - (tile._boundary._right - tile._out._right + 1.0f),
                              tile._out._top - tile._boundary._top + 1.0f),

                    tile._out._left - tile._boundary._left + 1.0f,
                    _dimensions.y - (tile._out._top - tile._boundary._top + 1.0f)
                                  - (tile._boundary._bottom - tile._out._bottom + 1.0f),

                    tile._out._right, tile._in._top,
                    tile._boundary._right, tile._in._bottom,
                    offset, _vertices);

                // top
                offset += makeQuad(
                    glm::vec2(tile._out._left - tile._boundary._left + 1.0f,
                              _dimensions.y - (tile._out._top - tile._boundary._top + 1.0f)),

                    _dimensions.x - (tile._out._left - tile._boundary._left + 1.0f)
                                  - (tile._boundary._right - tile._out._right + 1.0f),
                    tile._out._top - tile._boundary._top + 1.0f,

                    tile._in._left, tile._boundary._top,
                    tile._in._right, tile._out._top,
                    offset, _vertices);

                // center //

                offset += makeQuad(
                    glm::vec2((tile._out._left - tile._boundary._left + 1.0f),
                              (tile._out._top - tile._boundary._top + 1.0f)),

                    _dimensions.x - (tile._out._left - tile._boundary._left + 1.0f)
                                  - (tile._boundary._right - tile._out._right + 1.0f),
                    _dimensions.y - (tile._out._top - tile._boundary._top + 1.0f)
                                  - (tile._boundary._bottom - tile._out._bottom + 1.0f),

                    tile._in._left, tile._in._top,
                    tile._in._right, tile._in._bottom,
                    offset, _vertices);

                assert(offset == _vertices.size());
            }

        private:
            size_t makeQuad(glm::vec2 const & dP,
                            float left, float top, float right, float bottom,
                            size_t offset, std::vector<Vertex> & out) const
            {
                float const width = right - left + 1.0f;
                float const height = bottom - top + 1.0f;
                return makeQuad(dP, width, height,
                                left, top, right, bottom,
                                offset, out);
            }

            // TODO: don't need recalc UVs all the time
            size_t makeQuad(glm::vec2 const & dP, float width, float height,
                            float left, float top, float right, float bottom,
                            size_t offset, std::vector<Vertex> & out) const
            {
                if (!(width >= 0 && height >= 0 &&
                      left <= right && top <= bottom))
                {
                    MINIRE_THROW("bad quad: width = {}; height = {}; "
                                 "left = {}; right = {}; "
                                 "top = {}; bottom = {}; "
                                 "dimensions = [{}, {}]",
                                 width, height, left, right, top, bottom,
                                 _dimensions.x, _dimensions.y);
                }

                glm::vec2 const texMinSz(right - left + 1.0f, bottom - top + 1.0f);
                glm::vec2 const repeat = glm::vec2(width, height) / texMinSz;

                out[offset + 0]._pos = floor(_position + dP + glm::vec2(0.0, height));
                out[offset + 1]._pos = floor(_position + dP);
                out[offset + 2]._pos = floor(_position + dP + glm::vec2(width, 0.0));
                out[offset + 3]._pos = floor(_position + dP + glm::vec2(0.0, height));
                out[offset + 4]._pos = floor(_position + dP + glm::vec2(width, 0.0));
                out[offset + 5]._pos = floor(_position + dP + glm::vec2(width, height));

                out[offset + 0]._uv = glm::vec2(0, 0) * repeat;
                out[offset + 1]._uv = glm::vec2(0, 1) * repeat;
                out[offset + 2]._uv = glm::vec2(1, 1) * repeat;
                out[offset + 3]._uv = glm::vec2(0, 0) * repeat;
                out[offset + 4]._uv = glm::vec2(1, 1) * repeat;
                out[offset + 5]._uv = glm::vec2(1, 0) * repeat;

                for(int i(0); i < 6; ++i)
                {
                    out[offset + i]._rep = glm::vec2(left, top);
                    out[offset + i]._dims = texMinSz;
                }

                return 6;
            }

        private:
            Textures::Texture const & _texture;
            glm::vec2                 _position;
            glm::vec2                 _dimensions;
            std::vector<Vertex> &     _vertices;
        };
    }

    // Sprites::Sprite //

    class Sprites::Sprite : public Drawable
    {
    public:
        Sprite(Textures::Texture::Sptr texture,
               TileInfo tileInfo,
               glm::vec2 const & position,
               glm::vec2 const & dimensions,
               bool visible, int z,
               Program const & program)
            : Drawable(z)
            , _texture(texture)
            , _tileInfo(tileInfo)
            , _position(position)
            , _dimensions(dimensions)
            , _visible(visible)
            , _program(program)
            , _vao(std::make_shared<opengl::VAO>())
            , _vbo(_vao, GL_ARRAY_BUFFER)
            , _invalidated(true)
        {
            MINIRE_INVARIANT(_texture, "sprite created w/o a texture");

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

            // layout(location = 2) in vec2 bznkRep;
            _vao->enableAttrib(2);
            _vao->attribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, pointer);
            pointer += sizeof(Vertex::_rep);

            // layout(location = 3) in vec2 bznkDims;
            _vao->enableAttrib(3);
            _vao->attribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, pointer);
            pointer += sizeof(Vertex::_rep);

            // allocate storage
            _vertices.resize(getTileInfoSize(_tileInfo));
            _vbo.bufferData(_vertices.size() * sizeof(Vertex),
                            nullptr, GL_STATIC_DRAW);
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
            if (std::holds_alternative<utils::Rect>(_tileInfo))
            {
                MINIRE_THROW("should not set dimensions for sprite!");
            }

            _dimensions = d;
            _invalidated = true;
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

            MINIRE_GL(glActiveTexture, GL_TEXTURE0);
            _texture->bind();

            _vao->bind();
            _vbo.bind();

            MINIRE_GL(glDrawArrays, GL_TRIANGLES, 0, _vertices.size());
        }

    private:
        void revalidate() const
        {
            if (_invalidated)
            {
                assert(_texture);
                TileInfoVisitor visitor(*_texture, _position, _dimensions, _vertices);
                std::visit(visitor, _tileInfo);
                _vbo.bufferSubData(0, _vertices.size() * sizeof(Vertex),
                                   _vertices.data());
                _invalidated = false;
            }
        }

    private:
        Textures::Texture::Sptr     _texture;
        TileInfo                    _tileInfo;
        glm::vec2                   _position;
        glm::vec2                   _dimensions;
        bool                        _visible;
        Program const &             _program;

        mutable std::vector<Vertex> _vertices;
        mutable opengl::VAO::Sptr   _vao;
        mutable opengl::VBO         _vbo;
        mutable bool                _invalidated;
    };

    // Sprites //

    Sprites::Sprites(Textures const & textures)
        : _textures(textures)
        , _program(std::make_unique<Program>())
    {}

    Sprites::~Sprites() = default;
    
    void Sprites::create(std::string const & id,
                         content::Id const & texture,
                         utils::Rect const & tile,
                         glm::vec2 const & position,
                         bool const visible,
                         int const z)
    {
        auto res = _store.emplace(id,
            std::make_unique<Sprite>(_textures.getNoMipmap(texture), tile,
                                     position, glm::vec2(), visible, z,
                                     *_program));
        if (!res.second)
        {
            MINIRE_THROW("sprite alrady exists: \"{}\"", id);
        }
    }

    void Sprites::create(std::string const & id,
                         content::Id const & texture,
                         utils::NinePatch const & tile,
                         glm::vec2 const & position,
                         glm::vec2 const & dimensions,
                         bool const visible,
                         int const z)
    {
        auto res = _store.emplace(id,
            std::make_unique<Sprite>(_textures.getNoMipmap(texture), tile,
                                     position, dimensions, visible, z,
                                     *_program));
        if (!res.second)
        {
            MINIRE_THROW("sprite alrady exists: \"{}\"", id);
        }
    }

    void Sprites::move(std::string const & id,
                       glm::vec2 const & position)
    {
        find(id).setPosition(position);
    }

    void Sprites::resize(std::string const & id,
                         glm::vec2 const & dimensions)
    {
        find(id).setDimensions(dimensions);
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
        if (it == _store.cend()) MINIRE_THROW("no such sprite: \"{}\"", id);
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
