#include <minire/sdl/gl-application.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>
#include <minire/opengl.hpp>

namespace minire::sdl
{
    namespace
    {
        void GLAPIENTRY
        MessageCallback( GLenum /*source*/,
                         GLenum type,
                         GLuint /*id*/,
                         GLenum severity,
                         GLsizei /*length*/,
                         const GLchar* message,
                         const void* /*userParam*/ )
        {
            logging::Level level = GL_DEBUG_TYPE_ERROR == type ? logging::Level::kError
                                                               : logging::Level::kDebug;
            MINIRE_LOG(level, "type = {:#x}, severity = {:#x}, message = {} ",
                       type, severity, message);
        }

        void enableGlDebug()
        {
            ::glEnable(GL_DEBUG_OUTPUT);
            ::glDebugMessageCallback(MessageCallback, 0);
        }

        void disableGlDebug()
        {
            ::glDisable(GL_DEBUG_OUTPUT);
        }
    }

    GlApplication::GlApplication(int width, int height,
                                 std::string const & title)
        : Application(width, height, title)
        , _SDLGlContext()
    {
        try
        {
            //::SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            //::SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
            if (0 != ::SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                           SDL_GL_CONTEXT_PROFILE_CORE))
            {
                MINIRE_THROW("SDL_GL_SetAttribute failed: {}", ::SDL_GetError());
            }

            _SDLGlContext = ::SDL_GL_CreateContext(window());
            if (!_SDLGlContext)
            {
                MINIRE_THROW("SDL_GL_CreateContext failed: {}", ::SDL_GetError());
            }

            MINIRE_INFO("OpenGL: {}", (const char *) ::glGetString(GL_VERSION));
        }
        catch(...)
        {
            if (_SDLGlContext) ::SDL_GL_DeleteContext(_SDLGlContext);
            throw;
        }
#ifndef NDEBUG
        enableGlDebug();
#endif
    }

    GlApplication::~GlApplication()
    {
        if (_SDLGlContext) ::SDL_GL_DeleteContext(_SDLGlContext);
    }

    void GlApplication::setVsync(bool enabled) const
    {
        if (enabled)
        {
            if (::SDL_GL_SetSwapInterval(-1) == -1)
            {
                MINIRE_ERROR("SDL_GL_SetSwapInterval(-1) failed: {}, "
                             "will fallback to a regular vsync)",
                             ::SDL_GetError());
                if (::SDL_GL_SetSwapInterval(1) == -1)
                {
                    MINIRE_THROW("SDL_GL_SetSwapInterval(1) failed: {}", ::SDL_GetError());
                }
            }
        }
        else
        {
            if (::SDL_GL_SetSwapInterval(0) == -1)
            {
                MINIRE_THROW("SDL_GL_SetSwapInterval(0) failed: {}", ::SDL_GetError());
            }
        }
    }

    void GlApplication::setGlDebug(bool enabled) const
    {
        if (enabled)
        {
            enableGlDebug();
        }
        else
        {
            disableGlDebug();
        }
    }

}
