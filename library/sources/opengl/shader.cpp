#include <opengl/shader.hpp>

#include <minire/errors.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

namespace minire::opengl
{
    namespace
    {
        // TODO: tests
        std::string addLineNums(std::string const & in)
        {
            std::stringstream result;
            size_t cursor = 0, lineno = 0;

            while (cursor < in.size())
            {
                size_t pos = in.find('\n', cursor);
                if (pos == std::string::npos) pos = in.size();
                result << std::setw(5) << std::setfill('0')
                       << ++lineno << "|  " << in.substr(cursor, pos - cursor)
                       << '\n';
                cursor = pos + 1;
            }

            return result.str();
        }
    }

    /*!
     * see also https://www.opengl.org/wiki/Shader_Compilation
     **/
    Shader::Shader(GLenum type, std::string const & source)
        : _id(0)
        , _type(type)
    {
        try
        {
            // Create a shader
            _id = ::glCreateShader(type);
            if (0 == _id) MINIRE_THROW("glCreateShader returned 0");

            // Load shader's code
            const GLchar * sources[1] = {source.data()};
            MINIRE_GL(glShaderSource, _id, 1, sources, nullptr);

            // Compile the shader
            MINIRE_GL(glCompileShader, _id);

            // Check compilation status
            GLint compileStatus;
            MINIRE_GL(glGetShaderiv, _id, GL_COMPILE_STATUS, &compileStatus);

            if (GL_FALSE == compileStatus)
            {
                GLint logLength;
                MINIRE_GL(glGetShaderiv, _id, GL_INFO_LOG_LENGTH, &logLength);
                if (logLength > 0)
                {
                    std::vector<GLchar> buffer(logLength + 32, '\0');
                    MINIRE_GL(glGetShaderInfoLog, _id, buffer.size(), nullptr, buffer.data());
                    MINIRE_THROW("shader compilation error:\n{}\nSource code:\n{}",
                                 std::string(buffer.data()), addLineNums(source));
                }
            }
        }
        catch(...)
        {
            if (_id != 0) ::glDeleteShader(_id);
            throw;
        }
   }

    Shader::~Shader()
    {
        if (_id != 0 && GL_TRUE == ::glIsShader(_id))
        {
            ::glDeleteShader(_id);
        }
    }
}
