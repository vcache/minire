#include <rasterizer/lines.hpp>

#include <rasterizer/ubo.hpp>
#include <opengl.hpp>
#include <opengl/program.hpp>
#include <opengl/shader.hpp>
#include <opengl/vao.hpp>
#include <opengl/vbo.hpp>

#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr

#include <cassert>
#include <limits>

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
            void main()
            {
                gl_Position = _viewProjection *
                              vec4(bznkPos, 1.0);
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

    class Lines::Impl
    {
    public:
        explicit Impl(Ubo const & ubo)
            : _program({
                std::make_shared<opengl::Shader>(GL_VERTEX_SHADER, VertShader()),
                std::make_shared<opengl::Shader>(GL_FRAGMENT_SHADER, kFragShader)})
        {
            _program.use();
            ubo.bindBufferRange(_program);
        }

        void draw() const
        {
            if (_vbo && _vao)
            {
                _program.use();

                _vao->bind();
                _vbo->bind();

                MINIRE_GL(glDrawArrays, GL_LINES, 0, _vboCount);
            }
        }

        void update(std::vector<float> const & buffer)
        {
            size_t const bytes = buffer.size() * sizeof(float);
            assert(std::numeric_limits<GLsizeiptr>::max() > bytes);

            if (!_vao)
            {
                _vao = std::make_shared<opengl::VAO>();
                _vbo = std::make_shared<opengl::VBO>(_vao, GL_ARRAY_BUFFER);
                _vbo->bufferData(bytes, buffer.data(), GL_DYNAMIC_DRAW);
                reSetVaoPointers();
            }
            else if (_vbo->size() < static_cast<GLsizeiptr>(bytes))
            {
                assert(_vao);
                _vbo = std::make_shared<opengl::VBO>(_vao, GL_ARRAY_BUFFER);
                _vbo->bufferData(bytes, buffer.data(), GL_DYNAMIC_DRAW);
                reSetVaoPointers();
            }
            else
            {
                assert(_vao);
                assert(_vbo);
                _vbo->bufferSubData(0, bytes, buffer.data());
            }
            _vboCount = buffer.size() / 6;
        }

    private:
        void reSetVaoPointers()
        {
            constexpr size_t kStride = sizeof(float) * (3 + 3);
            assert(_vao);

            // position
            _vao->attribPointer(0, 3, GL_FLOAT, GL_FALSE, kStride, 0);
            _vao->enableAttrib(0);

            // color
            _vao->attribPointer(1, 3, GL_FLOAT, GL_FALSE, kStride,
                                3 * sizeof(float));
            _vao->enableAttrib(1);
        }

    private:
        opengl::Program   _program;
        opengl::VAO::Sptr _vao;
        opengl::VBO::Sptr _vbo;
        size_t            _vboCount;
    };

    Lines::Lines(Ubo const & ubo)
        : _impl(std::make_shared<Impl>(ubo))
    {}

    void Lines::update(std::vector<float> const & buffer)
    {
        assert(_impl);
        _impl->update(buffer);
    }

    void Lines::draw() const
    {
        assert(_impl);
        _impl->draw();
    }
}
