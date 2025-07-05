#pragma once

#include <opengl.hpp>

#include <memory>

namespace minire::opengl
{
    // TODO: why not moveable?
    class Shader
    {
    public:
        using Sptr = std::shared_ptr<Shader>;

        Shader(GLenum type, std::string const & source);
        ~Shader();

    private:
        GLuint getId() const { return _id; }

    private:
        GLuint _id;
        GLenum _type;

        friend class Program;

        Shader(Shader const &) = delete;
        Shader& operator=(Shader const &) = delete;
        Shader(Shader &&) = delete;
        Shader& operator=(Shader &&) = delete;
    };
}
