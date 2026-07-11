#pragma once

#include <opengl.hpp>

#include <cassert>
#include <memory>

namespace minire::opengl
{
    class SSBO
    {
        SSBO(SSBO const &) = delete;
        SSBO& operator=(SSBO const &) = delete;
 
    public:
        SSBO()
            : _ssbo(0)
            , _size(0)
        {
            try
            {
                MINIRE_GL(glGenBuffers, 1, &_ssbo);
                bind();
            }
            catch(...)
            {
                if (_ssbo == _used) _used = 0;
                ::glDeleteBuffers(1, &_ssbo);
                throw;
            }
        }

        SSBO(SSBO && other)
            : _ssbo(other._ssbo)
            , _size(other._size)
        {
            other._ssbo = 0;
            other._size = 0;
        }

        SSBO & operator=(SSBO && other)
        {
            SSBO tmp(std::move(other));
            std::swap(_ssbo, tmp._ssbo);
            std::swap(_size, tmp._size);
            return *this;
        }

        ~SSBO()
        {
            if (_ssbo == _used) _used = 0;
            ::glDeleteBuffers(1, &_ssbo);
        }

        void bind() const
        {
            if (_ssbo != _used)
            {
                MINIRE_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, _ssbo);
                _used = _ssbo;
            }
        }

        void bufferData(GLsizeiptr size,
                        void const * data,
                        GLenum usage = GL_DYNAMIC_DRAW)
        {
            bind();
            MINIRE_GL(glBufferData, GL_SHADER_STORAGE_BUFFER, size, data, usage);
            _size = size;
        }

        void bufferSubData(GLintptr offset,
                           GLsizeiptr size,
                           void const * data)
        {
            bind();
            MINIRE_GL(glBufferSubData, GL_SHADER_STORAGE_BUFFER, offset, size, data);
        }

        void bindBufferBase(GLuint bindingPoint)
        {
            MINIRE_GL(glBindBufferBase, GL_SHADER_STORAGE_BUFFER, bindingPoint, _ssbo);
            _used = _ssbo; // glBindBufferBase implicitly binds the buffer
        }

        GLsizeiptr size() const
        {
            return _size;
        }

        static void unbind()
        {
            MINIRE_GL(glBindBuffer, GL_SHADER_STORAGE_BUFFER, 0);
            _used = 0;
        }

    private:
        GLuint     _ssbo;
        GLsizeiptr _size;

        static GLuint _used;
    };
}