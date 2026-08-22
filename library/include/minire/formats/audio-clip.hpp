#pragma once

#include <memory>
#include <mutex>
#include <string>

struct Mix_Chunk;
struct Mix_Music;

namespace minire::content { class Manager; }
namespace minire::sdl { class AudioMixer; }

namespace minire::formats
{
    // These routines only make sense if an Application has been created
    // with an SDL_Mixer.

    // NOTE: All AudioClip instance must be BEFORE destruction of AudioMixer.

    class AudioClip
    {
        AudioClip(AudioClip const &) = delete;
        AudioClip & operator=(AudioClip const &) = delete;
        AudioClip(AudioClip &&) = delete;
        AudioClip & operator=(AudioClip &&) = delete;

    public:
        using Sptr = std::shared_ptr<AudioClip>;
        using Wptr = std::weak_ptr<AudioClip>;

        explicit AudioClip(std::string const &);

        ~AudioClip();

    private:
        ::Mix_Chunk * asChunk() const;
        ::Mix_Music * asMusic() const;

        std::string const & filename() const { return _filename; }

    private:
        std::string const      _filename;

        mutable ::Mix_Chunk *  _sdlChunk = nullptr;
        mutable ::Mix_Music *  _sdlMusic = nullptr;
        mutable std::once_flag _sdlChunkFlag;
        mutable std::once_flag _sdlMusicFlag;

        friend class minire::content::Manager;
        friend class minire::sdl::AudioMixer;
    };
}
