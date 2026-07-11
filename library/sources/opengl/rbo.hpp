#pragma once

#include <opengl.hpp>

#include <cassert>
#include <memory>

namespace minire::opengl
{
    class RBO
    {
        RBO(RBO const &) = delete;
        RBO& operator=(RBO const &) = delete;

    public:
        RBO()
            : _id(0)
        {
            MINIRE_GL(glGenRenderbuffers, 1, &_id);
            assert(_id != 0);
        }

        RBO(RBO && other)
            : _id(other._id)
        {
            other._id = 0;
        }

        RBO & operator=(RBO && other)
        {
            RBO tmp(std::move(other));
            std::swap(_id, tmp._id);
            return *this;
        }

        ~RBO()
        {
            if (_id == _used) _used = 0;
            ::glDeleteRenderbuffers(1, &_id);
        }

        using Uptr = std::unique_ptr<RBO>;

    public:
        void bind() const
        {
            if (_id != _used)
            {
                MINIRE_GL(glBindRenderbuffer, GL_RENDERBUFFER, _id);
                _used = _id;
            }
        }

        void storage(GLsizei width, GLsizei height,
                     GLenum internalFormat = GL_DEPTH_COMPONENT24)
        {
            bind();
            MINIRE_GL(glRenderbufferStorage, GL_RENDERBUFFER,
                      internalFormat, width, height);
        }

        static void unbind()
        {
            MINIRE_GL(glBindRenderbuffer, GL_RENDERBUFFER, 0);
            _used = 0;
        }

        GLuint id() const { return _id; }

    private:
        GLuint _id;

        static GLuint _used;
    };
}
