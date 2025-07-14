#include <rasterizer/coordinates.hpp>

#include <rasterizer/ubo.hpp>

#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>

#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr

#include <cassert>

namespace minire::rasterizer
{
    static std::string VertShader()
    {
        return
        R"(
            #version 330 core

            layout(location = 0) in vec3 bznkPos;
            layout(location = 1) in vec3 bznkCol;

            out vec3 bznkFragCol;
        )"
            + Ubo::interfaceBlock() +
        R"(
            uniform float bznkLength;

            void main()
            {
                gl_Position = _viewProjection *
                              vec4(bznkPos * bznkLength, 1.0);
                bznkFragCol = bznkCol;
            }
        )";
    }

    static const char * kFragShader = R"(
        #version 330 core

        in vec3 bznkFragCol;

        out vec3 bznkOutCol;

        void main()
        {
            bznkOutCol = bznkFragCol;
        }
    )";

    static const float kVbo[] = {
        0.0, 0.0, 0.0,  1.0, 0.0, 0.0, // x
        1.0, 0.0, 0.0,  1.0, 0.0, 0.0,

        0.0, 0.0, 0.0,  0.0, 1.0, 0.0, // y
        0.0, 1.0, 0.0,  0.0, 1.0, 0.0,

        0.0, 0.0, 0.0,  0.0, 0.0, 1.0, // z
        0.0, 0.0, 1.0,  0.0, 0.0, 1.0,
    };

    class Coordinates::Impl
    {
    public:
        Impl(Ubo const & ubo,
             float length)
            : _program({
                std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, VertShader()),
                std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader)})
            , _vao(std::make_shared<opengl::VAO>())
            , _vbo(_vao, GL_ARRAY_BUFFER)
            , _bznkLengthUniform(_program.getUniformLocation("bznkLength"))
        {
            _vbo.bufferData(sizeof(kVbo), kVbo, GL_STATIC_DRAW);

            constexpr size_t stride = sizeof(float) * (3 + 3);

            // position
            _vao->attribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
            _vao->enableAttrib(0);

            // color
            _vao->attribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, 3 * sizeof(float));
            _vao->enableAttrib(1);

            _program.use();
            setBznkLengthUniform(length);

            ubo.bindBufferRange(_program);
        }

        void draw() const
        {
            _program.use();

            assert(_vao);
            _vao->bind();
            _vbo.bind();

            MINIRE_GL(glDrawArrays, GL_LINES, 0, 6);
        }

        void setBznkLengthUniform(float length) const
        {
            MINIRE_GL(glUniform1f, _bznkLengthUniform, length);
        }

    private:
        opengl::Program   _program;
        opengl::VAO::Sptr _vao;
        opengl::VBO       _vbo;

        GLint             _bznkLengthUniform;
    };

    Coordinates::Coordinates(Ubo const & ubo,
                             float length)
        : _impl(std::make_shared<Impl>(ubo, length))
    {}

    void Coordinates::draw() const
    {
        assert(_impl);
        _impl->draw();
    }
}
