#include <opengl/program.hpp>

#include <minire/errors.hpp>

#include <vector>

namespace minire::opengl
{
    GLuint Program::_used = 0;

    Program::Program(std::vector<Shader::Sptr> shaders)
        : _shaders(std::move(shaders))
        , _id(0)
    {
        MINIRE_INVARIANT(!_shaders.empty(),
                         "no shaders provided to a program");

        try
        {
            // Create a program
            _id = ::glCreateProgram();
            if (0 == _id) MINIRE_THROW("glCreateProgram returned 0");

            // Load shaders
            for(const auto & i : _shaders)
            {
                if (!i)
                {
                    MINIRE_THROW("nullptr provided instead shader pointer");
                }

                auto shaderId = i->getId();
                if (shaderId == 0 || GL_FALSE == ::glIsShader(shaderId))
                {
                    MINIRE_THROW("bad shader prodvided to program: {}", shaderId);
                }

                MINIRE_GL(glAttachShader, _id, shaderId);
            }

            // Link the program
            MINIRE_GL(glLinkProgram, _id);

            GLint linkingStatus;
            MINIRE_GL(glGetProgramiv, _id, GL_LINK_STATUS, &linkingStatus); // TODO: this will put program in glUseProgram
            if (GL_FALSE == linkingStatus)
            {
                MINIRE_THROW("program linking error:\n{}", getInfoLog());
            }

            // Perform validation
            MINIRE_GL(glValidateProgram, _id);

            GLint valid;
            MINIRE_GL(glGetProgramiv, _id, GL_VALIDATE_STATUS, &valid);
            if (valid != GL_TRUE)
            {
                MINIRE_THROW("program Validation failed:\n{}", getInfoLog());
            }
        }
        catch(...)
        {
            if (_id != 0) ::glDeleteProgram(_id);
            throw;
        }
    }

    Program::~Program()
    {
        if (_id != 0 && GL_TRUE == ::glIsProgram(_id))
        {
            ::glDeleteProgram(_id);
        }
    }

    std::string Program::getInfoLog() const
    {
        GLint logLength;
        MINIRE_GL(glGetProgramiv, _id, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0)
        {
            std::vector<GLchar> buffer(logLength + 32, '\0');
            MINIRE_GL(glGetProgramInfoLog, _id, buffer.size(), nullptr, buffer.data());
            return std::string(buffer.data());
        }

        return std::string();
    }
}
