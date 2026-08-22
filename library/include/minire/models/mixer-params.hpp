#pragma once
 
#include <cstddef>

#include <SDL2/SDL_mixer.h>

namespace minire::models
{
    // NOTE: not creating mixer if _flags == 0
    struct MixerParams
    {
        int    _flags = 0;
        size_t _frequency = MIX_DEFAULT_FREQUENCY;
        size_t _format = MIX_DEFAULT_FORMAT;
        size_t _channels = MIX_DEFAULT_CHANNELS;    // mono/stereo
        size_t _chunksize = 2048;
        size_t _tracks = 32;
    };
}