#pragma once

#include <opengl.hpp>
#include <opengl/shader.hpp>

#include <vector>
#include <memory>

namespace minire::opengl
{
    class Program
    {
    public:
        using Sptr = std::shared_ptr<Program>;

        explicit Program(std::vector<Shader::Sptr>);
        ~Program();

        void use() const
        {
            MINIRE_GL(glUseProgram, _id);
            _used = _id;
        }

        GLint getUniformLocation(GLchar const * name) const
        {
            auto res = ::glGetUniformLocation(_id, name);
            MINIRE_MAYBE_THROW_GL("glGetUniformLocation");
            return res;
        }

        GLint getAttribLocation(GLchar const * name) const
        {
            auto res = ::glGetAttribLocation(_id, name);
            MINIRE_MAYBE_THROW_GL("glGetAttribLocation");
            return res;
        }

        GLuint getUniformBlockIndex(const GLchar * uniformBlockName) const
        {
            GLuint result = ::glGetUniformBlockIndex(_id, uniformBlockName);
            MINIRE_MAYBE_THROW_GL(glGetUniformBlockIndex);
            return result;
        }

        GLuint id() const { return _id; }

        bool isUsing() const { return _id == _used; }

        template<class T>
        void setUniformValue(GLint, T);

    private:
        std::vector<Shader::Sptr> _shaders;
        GLuint                    _id;

        static GLuint             _used;

    private:
        std::string getInfoLog() const;
    };
}
