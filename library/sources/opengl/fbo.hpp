#pragma once

#include <opengl.hpp>
#include <opengl/rbo.hpp>
#include <opengl/texture.hpp>

#include <cassert>
#include <utility>

namespace minire::opengl
{
    class FBO
    {
        FBO(FBO const &) = delete;
        FBO& operator=(FBO const &) = delete;

    public:
        FBO()
            : _id(0)
        {
            MINIRE_GL(glGenFramebuffers, 1, &_id);
            assert(_id != 0);
        }

        FBO(FBO && other)
            : _id(other._id)
        {
            other._id = 0;
        }

        FBO & operator=(FBO && other)
        {
            FBO tmp(std::move(other));
            std::swap(_id, tmp._id);
            return *this;
        }

        ~FBO()
        {
            if (_id == _used) _used = 0;
            ::glDeleteFramebuffers(1, &_id);
        }

    public:
        void bind(GLuint framebuffer = GL_FRAMEBUFFER) const
        {
            if (_id != _used)
            {
                MINIRE_GL(glBindFramebuffer, framebuffer, _id);
                _used = _id;
            }
        }

        void attach(Texture const & depthTexture,
                    GLenum attachment) const
        {
            bind();
            MINIRE_GL(glFramebufferTexture, GL_FRAMEBUFFER, attachment,
                      depthTexture.id(), 0);
        }

        void attach2D(Texture const & depthTexture,
                      GLenum attachment) const
        {
            bind();
            MINIRE_GL(glFramebufferTexture2D, GL_FRAMEBUFFER, attachment,
                      depthTexture.target(), depthTexture.id(), 0);
        }

        void attach(RBO const & rbo, GLenum attachment)
        {
            bind();
            MINIRE_GL(glFramebufferRenderbuffer, GL_FRAMEBUFFER,
                      attachment, GL_RENDERBUFFER, rbo.id());
        }

        static void unbind()
        {
            MINIRE_GL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
            _used = 0;
        }

        GLuint id() const { return _id; }

    private:
        GLuint _id;

        static GLuint _used;
    };
}
