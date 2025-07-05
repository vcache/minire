#pragma once

#include <opengl.hpp>

namespace minire::opengl
{
    template<typename Struct>
    class UBO
    {
        UBO(UBO const &) = delete;
        UBO & operator=(UBO const &) = delete;

    public:
        UBO() : _buffer(0)
        {
            static_assert((sizeof(Struct) % 4) == 0,
                          "UBO data must be divisabe by 4");

            MINIRE_GL(glGenBuffers, 1, &_buffer);
            MINIRE_GL(glBindBuffer, GL_UNIFORM_BUFFER, _buffer);
            MINIRE_GL(glBufferData,
                      GL_UNIFORM_BUFFER,
                      sizeof(Struct),
                      nullptr,
                      GL_STATIC_DRAW);
        }

        UBO(UBO && other) : _buffer(other._buffer)
        {
            other._buffer = 0;
        }

        ~UBO()
        {
            ::glDeleteBuffers(1, &_buffer);
        }

        UBO & operator=(UBO && other)
        {
            UBO tmp(std::move(other));
            std::swap(_buffer, tmp._buffer);
            return *this;
        }

    public:
        void bind() const
        {
            MINIRE_GL(glBindBuffer, GL_UNIFORM_BUFFER, _buffer);
        }

        void update(Struct const & data) const
        {
            bind();
            MINIRE_GL(glBufferSubData,
                      GL_UNIFORM_BUFFER, 0, sizeof(Struct), &data);
        }

        void bindBufferBase(GLuint index) const
        {
            MINIRE_GL(glBindBufferBase, GL_UNIFORM_BUFFER, index, _buffer);
        }

    private:
        GLuint _buffer;
    };
}
