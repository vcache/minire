#pragma once

#include <minire/errors.hpp>

#include <opengl.hpp>

#include <cassert>

namespace minire::opengl
{
    class VBO
    {
        VBO(VBO const &) = delete;
        VBO& operator=(VBO const &) = delete;

    public:
        /*
         * \note Ctor will bind the buffer as a side effect
         */
        explicit VBO(GLenum target)
            : _vboId(0)
            , _target(target)
            , _size(0)
        {
            try
            {
                MINIRE_GL(glGenBuffers, 1, &_vboId);
                bind();
            }
            catch(...)
            {
                glDeleteBuffers(1, &_vboId);
                throw;
            }
        }

        ~VBO()
        {
            ::glDeleteBuffers(1, &_vboId);
        }

        VBO(VBO && other)
            : _vboId(other._vboId)
            , _target(other._target)
            , _size(other._size)
        {
            other._vboId = 0;
            other._target = 0;
            other._size = 0;
        }

        VBO & operator=(VBO && other)
        {
            VBO tmp(std::move(other));
            std::swap(_vboId, tmp._vboId);
            std::swap(_target, tmp._target);
            std::swap(_size, tmp._size);
            return *this;
        }

    public:
        void bind() const
        {
            MINIRE_GL(glBindBuffer, _target, _vboId);
        }

        void bufferData(GLsizeiptr size,
                        void const * data,
                        GLenum usage)
        {
            bind();
            MINIRE_GL(glBufferData, _target, size, data, usage);
            _size = size;
        }

        void bufferSubData(GLintptr offset,
                           GLsizeiptr size,
                           const void * data)
        {
            bind();
            MINIRE_GL(glBufferSubData, _target, offset, size, data);
        }

        GLsizeiptr size() const
        {
            return _size;
        }

        GLuint id() const { return _vboId; }

        GLenum target() const { return _target; }

    private:
        GLuint     _vboId;
        GLenum     _target;
        GLsizeiptr _size;
    };
}
