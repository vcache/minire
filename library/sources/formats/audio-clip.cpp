#include <minire/formats/audio-clip.hpp>

#include <minire/errors.hpp>

#include <cassert>

#include <SDL2/SDL_mixer.h>

namespace minire::formats
{
    AudioClip::AudioClip(std::string const & filename)
        : _filename(filename)
    {}

    ::Mix_Chunk * AudioClip::asChunk() const
    {
        std::call_once(_sdlChunkFlag, [this]
            {
                _sdlChunk = ::Mix_LoadWAV(_filename.c_str());
                MINIRE_INVARIANT(_sdlChunk, "Mix_LoadWAV failed: {}", ::Mix_GetError());
            });
        assert(_sdlChunk);
        return _sdlChunk;
    }

    ::Mix_Music * AudioClip::asMusic() const
    {
        std::call_once(_sdlMusicFlag, [this]
            {
                _sdlMusic = ::Mix_LoadMUS(_filename.c_str());
                MINIRE_INVARIANT(_sdlMusic, "Mix_LoadMUS failed: {}", ::Mix_GetError());
            });
        assert(_sdlMusic);
        return _sdlMusic;
    }

    // Assuming no users AudioClip by the time it will be destroyed,
    // so that, it must be thread-safe.
    AudioClip::~AudioClip()
    {
        if (_sdlChunk)
        {
            ::Mix_FreeChunk(_sdlChunk);
        }

        if (_sdlMusic)
        {
            ::Mix_FreeMusic(_sdlMusic);
        }
    }
}
