#pragma once

#include <minire/sdl/application.hpp>

#include <SDL2/SDL.h>

#include <string>

namespace minire::sdl
{
    class GlApplication : public Application
    {
    public:
        GlApplication(int width, int height,
                      std::string const & title);
        ~GlApplication() override;

        void setVsync(bool enabled) const;

        void setGlDebug(bool enabled) const;

    private:
        ::SDL_GLContext _SDLGlContext;
    };
}
