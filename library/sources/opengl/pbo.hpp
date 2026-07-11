#pragma once

#include <minire/errors.hpp>

#include <opengl.hpp>

#include <cassert>
#include <memory>

namespace minire::opengl
{
    class PBO
    {
        PBO(PBO const &) = delete;
        PBO& operator=(PBO const &) = delete;

    public:
        using Uptr = std::unique_ptr<PBO>;

        explicit PBO()
            : _id(0)
        {
            try
            {
                MINIRE_GL(glGenBuffers, 1, &_id);
                bind(); // bind to associate w/ PIXEL_PACK_BUFFER
                assert(_id != 0);
            }
            catch(...)
            {
                if (_id == _used) _used = 0;
                ::glDeleteBuffers(1, &_id);
                throw;
            }
        }

        PBO(PBO && other)
            : _id(other._id)
        {
            other._id = 0;
        }

        PBO & operator=(PBO && other)
        {
            PBO tmp(std::move(other));
            std::swap(_id, tmp._id);
            return *this;
        }

        ~PBO()
        {
            if (_id == _used) _used = 0;
            ::glDeleteBuffers(1, &_id);
        }

    public:
        void bind()
        {
            if (_id != _used)
            {
                MINIRE_GL(glBindBuffer, GL_PIXEL_PACK_BUFFER, _id);
                _used = _id;
            }
        }

        void bufferData(GLsizeiptr size,
                        const GLvoid *data,
                        GLenum usage)
        {
            bind();
            MINIRE_GL(glBufferData, GL_PIXEL_PACK_BUFFER, size, data, usage);
        }

        void unbind()
        {
            MINIRE_GL(glBindBuffer, GL_PIXEL_PACK_BUFFER, 0);
            _used = 0;
        }

    public:
        class Mapping
        {
            Mapping(Mapping const &) = delete;
            Mapping & operator=(Mapping const &) = delete;
            Mapping(Mapping &&) = delete;
            Mapping & operator=(Mapping &&) = delete;

        public:
            Mapping()
                : _data(::glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY))
            {
                MINIRE_MAYBE_THROW_GL("glMapBuffer");
            }

            ~Mapping()
            {
                MINIRE_GL(glUnmapBuffer, GL_PIXEL_PACK_BUFFER);
            }

            void const * data() const { assert(_data); return _data; }

            template<typename T>
            T const * dataAs() const { assert(_data); return reinterpret_cast<T const *>(_data); }

            operator bool() const { return _data != nullptr; }

        private:
            void * _data = nullptr;
        };

        Mapping mapBuffer()
        {
            bind();
            return Mapping();
        }

    public:
        GLuint id() const { return _id; }

    private:
        GLuint _id;

        static GLuint _used;
    };
}
