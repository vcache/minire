#pragma once

#include <minire/errors.hpp>

#include <opengl.hpp>

#include <cassert>
#include <stdexcept>

namespace minire::opengl
{
    class VAO
    {
        VAO(VAO const &) = delete;
        VAO & operator=(VAO const &) = delete;

    public:
        VAO()
            : _vaoId(0)
        {
            try
            {
                MINIRE_GL(glGenVertexArrays, 1, &_vaoId);
                bind();
            }
            catch(...)
            {
                if (_vaoId == _used) _used = 0;
                ::glDeleteVertexArrays(1, &_vaoId);
            }
        }

        ~VAO()
        {
            if (_vaoId == _used) _used = 0;
            ::glDeleteVertexArrays(1, &_vaoId);
        }

        VAO(VAO && other)
            : _vaoId(other._vaoId)
        {
            other._vaoId = 0;
        }

        VAO & operator=(VAO && other)
        {
            VAO tmp(std::move(other));
            std::swap(_vaoId, tmp._vaoId);
            return *this;
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

        void attribDivisor(GLuint index, GLuint divisor) const
        {
            bind();
            MINIRE_GL(glVertexAttribDivisor, index, divisor);
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
