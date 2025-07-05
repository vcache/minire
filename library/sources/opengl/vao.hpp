#pragma once

#include <minire/errors.hpp>

#include <opengl.hpp>

#include <cassert>
#include <memory>
#include <stdexcept>

namespace minire::opengl
{
    class VAO
    {
    public:
        using Sptr = std::shared_ptr<VAO>;
        using Uptr = std::unique_ptr<VAO>;

        VAO()
            : _vaoId(0)
        {
            MINIRE_GL(glGenVertexArrays, 1, &_vaoId);

            bind();

            if (GL_FALSE == glIsVertexArray(_vaoId))
            {
                MINIRE_THROW("failed to generate VAO");
            }
        }

        ~VAO()
        {
            if (glIsVertexArray(_vaoId))
            {
                glDeleteVertexArrays(1, &_vaoId);
            }
        }

        void bind() const
        {
            MINIRE_GL(glBindVertexArray, _vaoId);
        }

        void enableAttrib(GLuint index) const
        {
            bind();
            MINIRE_GL(glEnableVertexAttribArray, index);
        }

        void disableAttrib(GLuint index) const
        {
            bind();
            MINIRE_GL(glDisableVertexAttribArray, index);
        }

        void attribPointer(GLuint index,
                           GLint size,
                           GLenum type,
                           GLboolean normalized,
                           GLsizei stride,
                           size_t pointer) const
        {
            bind();
            MINIRE_GL(glVertexAttribPointer,
                      index, size, type, normalized, stride,
                      reinterpret_cast<const GLvoid*>(pointer));
        }

        void attribIPointer(GLuint index,
                            GLint size,
                            GLenum type,
                            GLsizei stride,
                            size_t pointer) const
        {
            bind();
            MINIRE_GL(glVertexAttribIPointer,
                      index, size, type, stride,
                      reinterpret_cast<const GLvoid*>(pointer));
        }

        static void unbind()
        {
            MINIRE_GL(glBindVertexArray, 0);
        }

    private:
        GLuint _vaoId;
    };
}
