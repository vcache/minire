#pragma once

#include <minire/errors.hpp>

#include <opengl.hpp>
#include <opengl/vao.hpp>

#include <cassert>
#include <memory>

namespace minire::opengl
{
    class VBO
    {
    public:
        using Sptr = std::shared_ptr<VBO>;

        /*
         * \note Ctor will bind the buffer as a side effect
         */
        explicit VBO(VAO::Sptr const & vao, GLenum target)
            : _vboId(0)
            , _vao(vao)
            , _target(target)
            , _size(0)
        {
            if (!_vao)
            {
                MINIRE_THROW("vao is nullptr");
            }

            try
            {
                _vao->bind();

                MINIRE_GL(glGenBuffers, 1, &_vboId);

                bind();

                if (!glIsBuffer(_vboId))
                {
                    MINIRE_THROW("allocated VBO is not recognized as a buffer");
                }
            }
            catch(...)
            {
                if (_vboId > 0)
                {
                    glDeleteBuffers(1, &_vboId);
                }

                throw;
            }
        }

        virtual ~VBO() // TODO: why virtual?
        {
            if (glIsBuffer(_vboId))
            {
                glDeleteBuffers(1, &_vboId);
            }
        }

        VBO(VBO const &) = delete;
        VBO& operator=(VBO const &) = delete;

        VBO(VBO && other)
            : _vboId(other._vboId)
            , _vao(other._vao)
            , _target(other._target)
            , _size(other._size)
        {
            other._vboId = 0;
            other._vao = nullptr;
            other._target = 0;
            other._size = 0;
        }

        VBO & operator=(VBO && other)
        {
            VBO tmp(std::move(other));
            std::swap(_vboId, tmp._vboId);
            std::swap(_vao, tmp._vao);
            std::swap(_target, tmp._target);
            std::swap(_size, tmp._size);
            return *this;
        }

    public:
        void bind() const
        {
            assert(_vao);
            _vao->bind();
            MINIRE_GL(glBindBuffer, _target, _vboId);
        }

        void bufferData(GLsizeiptr size,
                        const GLvoid *data,
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
        VAO::Sptr  _vao;
        GLenum     _target;
        GLsizeiptr _size;
    };
}
