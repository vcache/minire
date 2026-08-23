#include <minire/formats/audio-clip.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <algorithm>
#include <cassert>

#include <SDL2/SDL_mixer.h>

namespace minire::formats
{
    namespace
    {
        float volumeToFloat(int const volume)
        {
            return static_cast<float>(volume) / static_cast<float>(MIX_MAX_VOLUME);
        }

        int volumeToInteger(float const volume)
        {
            return std::lround(static_cast<float>(MIX_MAX_VOLUME) *
                                           std::clamp(volume, 0.0f, 1.0f));
        }
    }

    AudioClip::AudioClip(std::string const & filename)
        : _filename(filename)
    {}

    ::Mix_Chunk * AudioClip::asChunk() const
    {
        std::call_once(_sdlChunkFlag, [this]
            {
                _sdlChunk = ::Mix_LoadWAV(_filename.c_str());
                MINIRE_INVARIANT(_sdlChunk, "Mix_LoadWAV failed: {}", ::Mix_GetError());
                ::Mix_VolumeChunk(_sdlChunk, _volume);
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

    float AudioClip::volume() const
    {
        if (_volume < 0)
            _volume = _sdlChunk ? ::Mix_VolumeChunk(_sdlChunk, -1)
                                : MIX_MAX_VOLUME; // default volume of SDL_Chunk
        return volumeToFloat(_volume);
    }

    void AudioClip::setVolume(float volume)
    {
        int const newVolume = volumeToInteger(volume);
        if (_sdlChunk && (_volume < 0 || newVolume != _volume))
        {
            ::Mix_VolumeChunk(_sdlChunk, newVolume);
        }

        if (_sdlMusic)
        {
            MINIRE_WARNING("setVolume will be ignored for streaming");
        }

        _volume = newVolume;
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
