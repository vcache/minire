#pragma once

// TODO: http://stackoverflow.com/questions/3032386/glgenbuffers-not-defined
#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glext.h>

#include <string_view>

namespace minire::opengl
{
    std::string_view errorToString(GLenum const errorCode);

    void maybeThrowGlError(const char * glCallName,
                           int line,
                           char const * file,
                           char const * prettyFunction);
}

#define MINIRE_MAYBE_THROW_GL(func) \
    ::minire::opengl::maybeThrowGlError(#func, __LINE__, __FILE__, __PRETTY_FUNCTION__);

#define MINIRE_GL(func, ...) {      \
    ::func(__VA_ARGS__);            \
    MINIRE_MAYBE_THROW_GL(#func);   \
}
