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

            MINIRE_INVARIANT(GL_TRUE == glIsVertexArray(_vaoId),
                             "failed to generate VAO: {}", _vaoId);
        }

        VAO(VAO const &) = delete;
        VAO(VAO &&) = delete;
        VAO & operator=(VAO const &) = delete;
        VAO & operator=(VAO &&) = delete;

        ~VAO()
        {
            if (glIsVertexArray(_vaoId))
            {
                if (_vaoId == _used)
                {
                    _used = 0;
                }
                glDeleteVertexArrays(1, &_vaoId);
            }
        }

        // NOTE: Avoid direct call to glBindVertexArray,
        //       since it will break synchronization!
        void bind() const
        {
            if (_used != _vaoId)
            {
                MINIRE_GL(glBindVertexArray, _vaoId);
                _used = _vaoId;
            }
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
            if (_used)
            {
                MINIRE_GL(glBindVertexArray, 0);
                _used = 0;
            }
        }

    private:
        GLuint _vaoId;

        static GLuint _used;
    };
}
