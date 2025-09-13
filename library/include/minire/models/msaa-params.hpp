#pragma once

#include <cstddef>

namespace minire::models
{
    struct MsaaParams
    {
        size_t _buffers = 0;    // SDL_GL_MULTISAMPLEBUFFERS
        size_t _samples = 1;    // SDL_GL_MULTISAMPLESAMPLES
    };
}
