#pragma once

#include <opengl.hpp>
#include <minire/models/image.hpp>

#include <memory>

namespace minire::opengl
{
    class Texture
    {
    public:
        using Uptr = std::unique_ptr<Texture>;

        explicit Texture(GLenum target)
            : _id(0)
            , _target(target)
        {
            MINIRE_GL(glGenTextures, 1, &_id);
            bind(); // NOTE: Don't remove! Some code depends
                    //       on bind() in a ctor.
        }

        ~Texture()
        {
            ::glDeleteTextures(1, &_id);
        }

        Texture(Texture && other)
            : _id(other._id)
            , _target(other._target)
        {
            other._id = other._target = 0;
        }

        Texture& operator=(Texture && other)
        {
            Texture tmp(std::move(other));
            swap(tmp);
            return *this;
        }

        void swap(Texture & other)
        {
            std::swap(_id, other._id);
            std::swap(_target, other._target);
        }

    public:
        GLuint id() const { return _id; }

        GLenum target() const { return _target; }

        void bind() const { MINIRE_GL(glBindTexture, _target, _id); }

        void parameteri(GLenum pname, GLint param) const
        {
            bind();
            MINIRE_GL(glTexParameteri, _target, pname, param);
        }

        std::pair<size_t, size_t> size(GLint miplevel = 0) const;

        GLint internalFormat() const;

    private:
        GLuint _id;
        GLenum _target;

        Texture(Texture const &) = delete;
        Texture& operator=(Texture const&) = delete;
    };

    GLenum toInternalFormat(models::Image::Format format);

    GLenum toFormat(models::Image::Format format);
}
