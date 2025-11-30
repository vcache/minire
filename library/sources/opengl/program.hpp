#pragma once

#include <opengl.hpp>
#include <opengl/shader.hpp>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp> // for gln::value_ptr

#include <array>
#include <cassert>
#include <vector>
#include <memory>

namespace minire::opengl
{
    class Program
    {
        Program(Program const &) = delete;
        Program & operator=(Program const &) = delete;

    public:
        using Sptr = std::shared_ptr<Program>;

        explicit Program(std::vector<Shader::Sptr>);
        ~Program();

        Program(Program &&);

        Program & operator=(Program &&);

    public:
        // NOTE: Avoid direct call to glUseProgram,
        //       since it will break synchronization!
        void use() const
        {
            if (_used != _id)
            {
                MINIRE_GL(glUseProgram, _id);
                _used = _id;
            }
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

        GLuint getUniformBlockIndex(GLchar const * uniformBlockName) const
        {
            GLuint result = ::glGetUniformBlockIndex(_id, uniformBlockName);
            MINIRE_MAYBE_THROW_GL(glGetUniformBlockIndex);
            return result;
        }

        GLuint id() const { return _id; }

        bool isUsing() const { return _id == _used; }

    public:
        void setUniform(GLint location, GLint value) const
        {
            assert(isUsing());
            MINIRE_GL(glUniform1i, location, value);
        }

        void setUniform(GLint location, float value) const
        {
            assert(isUsing());
            MINIRE_GL(glUniform1f, location, value);
        }

        void setUniform(GLint location, glm::vec2 const & value) const
        {
            assert(isUsing());
            MINIRE_GL(glUniform2f, location, value.x, value.y);
        }

        void setUniform(GLint location, glm::vec3 const & value) const
        {
            assert(isUsing());
            MINIRE_GL(glUniform3f, location, value.x, value.y, value.z);
        }

        void setUniform(GLint location, glm::mat4 const & value) const
        {
            assert(isUsing());
            MINIRE_GL(glUniformMatrix4fv, location, 1, GL_FALSE, glm::value_ptr(value));
        }

        template<size_t kItems>
        void setUniform(GLint location, std::array<glm::mat4, kItems> const & value) const
        {
            static_assert(kItems > 0, "number of matrices must be non-zero");
            assert(isUsing());
            MINIRE_GL(glUniformMatrix4fv, location, kItems, GL_FALSE, glm::value_ptr(value[0]));
        }

        template<size_t kItems>
        void setUniform(GLint location, std::array<GLint, kItems> const & value) const
        {
            static_assert(kItems > 0, "number of items must be non-zero");
            assert(isUsing());
            MINIRE_GL(glUniform1iv, location, value.size(), value.data());
        }

    private:
        std::vector<Shader::Sptr> _shaders;
        GLuint                    _id = 0;

        static GLuint             _used;

    private:
        std::string getInfoLog() const;
    };
}
