#include <opengl.hpp>

#include <minire/errors.hpp>

namespace minire::opengl
{
    std::string_view errorToString(GLenum const errorCode)
    {
#       define __MINIRE_GL_ENUM_CASE(ec) case ec: return #ec
        switch(errorCode)
        {
            __MINIRE_GL_ENUM_CASE(GL_NO_ERROR);
            __MINIRE_GL_ENUM_CASE(GL_INVALID_ENUM);
            __MINIRE_GL_ENUM_CASE(GL_INVALID_VALUE);
            __MINIRE_GL_ENUM_CASE(GL_INVALID_OPERATION);
            __MINIRE_GL_ENUM_CASE(GL_INVALID_FRAMEBUFFER_OPERATION);
            __MINIRE_GL_ENUM_CASE(GL_OUT_OF_MEMORY);
            __MINIRE_GL_ENUM_CASE(GL_STACK_UNDERFLOW);
            __MINIRE_GL_ENUM_CASE(GL_STACK_OVERFLOW);

            default: return "(unrecognized)";
        }
#       undef __MINIRE_GL_ENUM_CASE
    }

    void maybeThrowGlError(const char * glCallName,
                           int line,
                           const char * file,
                           char const * prettyFunction)
    {
        if (auto const err = ::glGetError();
            GL_NO_ERROR != err)
        {
            throw ::minire::RuntimeError(
                line, file, prettyFunction,
                ::minire::formatNoExc("OpengGL call {} failed: {}",
                                      glCallName, errorToString(err)));
        }
    }
}
