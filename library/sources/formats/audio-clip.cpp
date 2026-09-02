#include <minire/formats/audio-clip.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>

#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_rwops.h>

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

        Sint64 SDLCALL streamSize(SDL_RWops * context)
        {
            auto * stream = static_cast<std::istream *>(context->hidden.unknown.data1);
            if (!stream)
            {
                return -1;
            }
            stream->clear();
            std::streampos const current = stream->tellg();
            if (current == std::streampos(-1))
            {
                return -1;
            }
            stream->seekg(0, std::ios_base::end);
            std::streampos const end = stream->tellg();
            stream->seekg(current, std::ios_base::beg);
            return static_cast<Sint64>(end);
        }

        Sint64 SDLCALL streamSeek(SDL_RWops * context, Sint64 offset, int whence)
        {
            auto * stream = static_cast<std::istream *>(context->hidden.unknown.data1);
            if (!stream)
            {
                return -1;
            }
            stream->clear();
            std::ios_base::seekdir dir;
            switch (whence)
            {
                case RW_SEEK_SET:
                    dir = std::ios_base::beg;
                    break;
                case RW_SEEK_CUR:
                    dir = std::ios_base::cur;
                    break;
                case RW_SEEK_END:
                    dir = std::ios_base::end;
                    break;
                default:
                    return -1;
            }
            stream->seekg(offset, dir);
            if (stream->fail())
            {
                return -1;
            }
            std::streampos const pos = stream->tellg();
            return (pos == std::streampos(-1)) ? -1 : static_cast<Sint64>(pos);
        }

        size_t SDLCALL streamRead(SDL_RWops * context, void * ptr, size_t size, size_t maxnum)
        {
            auto * stream = static_cast<std::istream *>(context->hidden.unknown.data1);
            if (!stream || size == 0 || maxnum == 0)
            {
                return 0;
            }
            stream->clear();
            stream->read(static_cast<char *>(ptr), static_cast<std::streamsize>(size * maxnum));
            std::streamsize const bytesRead = stream->gcount();
            return static_cast<size_t>(bytesRead / size);
        }

        size_t SDLCALL streamWrite(SDL_RWops * /*context*/, const void * /*ptr*/, size_t /*size*/, size_t /*num*/)
        {
            return 0;
        }

        int SDLCALL streamClose(SDL_RWops * context)
        {
            if (context)
            {
                SDL_FreeRW(context);
            }
            return 0;
        }

        SDL_RWops * createStreamRWops(std::istream & stream)
        {
            SDL_RWops * rwops = ::SDL_AllocRW();
            if (!rwops)
            {
                return nullptr;
            }
            rwops->size = streamSize;
            rwops->seek = streamSeek;
            rwops->read = streamRead;
            rwops->write = streamWrite;
            rwops->close = streamClose;
            rwops->type = SDL_RWOPS_UNKNOWN;
            rwops->hidden.unknown.data1 = &stream;
            return rwops;
        }
    }

    AudioClip::AudioClip(std::string const & filename)
        : _filename(filename)
    {
        MINIRE_INVARIANT(!_filename.empty(), "no input filename provided");
    }

    AudioClip::AudioClip(std::unique_ptr<std::istream> istream)
        : _istream(std::move(istream))
    {
        MINIRE_INVARIANT(_istream, "no input stream provided");
    }

    ::Mix_Chunk * AudioClip::asChunk() const
    {
        std::call_once(_sdlChunkFlag, [this]
            {
                MINIRE_INVARIANT(!_sdlMusic,
                                 "the AudioClip is already used as Music, "
                                 "it cannot be re-purposed");

                assert(!_sdlChunk);

                if (_istream)
                {
                    assert(_filename.empty());
                    _istream->clear();
                    _istream->seekg(0, std::ios_base::beg);
                    SDL_RWops * const rwops = createStreamRWops(*_istream);
                    MINIRE_INVARIANT(rwops, "SDL_AllocRW failed: {}", ::SDL_GetError());
                    _sdlChunk = ::Mix_LoadWAV_RW(rwops, 1);
                }
                else
                {
                    assert(!_filename.empty());
                    _sdlChunk = ::Mix_LoadWAV(_filename.c_str());
                }

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
                MINIRE_INVARIANT(!_sdlChunk,
                                 "the AudioClip is already used as Chunk, "
                                 "it cannot be re-purposed");

                assert(!_sdlMusic);

                if (_istream)
                {
                    assert(_filename.empty());
                    _istream->clear();
                    _istream->seekg(0, std::ios_base::beg);
                    SDL_RWops * const rwops = createStreamRWops(*_istream);
                    MINIRE_INVARIANT(rwops, "SDL_AllocRW failed: {}", ::SDL_GetError());
                    _sdlMusic = ::Mix_LoadMUS_RW(rwops, 1);
                }
                else
                {
                    assert(!_filename.empty());
                    _sdlMusic = ::Mix_LoadMUS(_filename.c_str());
                }

                MINIRE_INVARIANT(_sdlMusic, "Mix_LoadMUS failed: {}", ::Mix_GetError());
                // NOTE: cannot set volume for an instance of Music
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