#include <minire/sdl/audio-mixer.hpp>

#include <minire/errors.hpp>
#include <minire/formats/audio-clip.hpp>
#include <minire/logging.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <algorithm>
#include <cassert>
#include <optional>

namespace minire::sdl
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

        template<typename T,
                 typename Deleter>
        class RaiiWrapper
        {
            RaiiWrapper(RaiiWrapper const &) = delete;
            RaiiWrapper& operator=(RaiiWrapper const &) = delete;

        public:
            template<typename U>
            explicit RaiiWrapper(U && data, Deleter deleter)
                : _data(std::forward<U>(data))
                , _deleter(deleter)
            {}

            RaiiWrapper(RaiiWrapper && other)
                : _data(std::move(other._data))
                , _deleter(std::move(other._deleter))
            {}

            RaiiWrapper& operator=(RaiiWrapper && other)
            {
                RaiiWrapper tmp(std::move(other));
                std::swap(_data, tmp._data);
                std::swap(_deleter, tmp._deleter);
                return *this;
            }

            ~RaiiWrapper()
            {
                if (_data)
                {
                    _deleter(std::move(*_data));
                    _data.reset();
                }
            }

            T release()
            {
                T result = *_data;
                _data.reset();
                return result;
            }

            T * operator->() noexcept { return _data.operator->(); }
            T const * operator->() const noexcept { return _data.operator->(); }

        private:
            std::optional<T> _data;
            Deleter          _deleter;
        };
    }

    // AudioMixer::Player //

    AudioMixer::Player::Player(Player && other) noexcept
        : _pool(std::move(other._pool))
        , _channel(other._channel)
        , _volume(other._volume)
    {
        other._pool.reset();
        other._channel = -1;
        other._volume = -1;
    }

    AudioMixer::Player& AudioMixer::Player::operator=(Player && other) noexcept
    {
        Player tmp(std::move(other));
        std::swap(_pool, tmp._pool);
        std::swap(_channel, tmp._channel);
        std::swap(_volume, tmp._volume);
        return *this;
    }

    AudioMixer::Player::~Player()
    {
        if (auto pool = _pool.lock(); pool)
        {
            assert(_channel >= 0);
            if (0 != ::Mix_HaltChannel(_channel))
            {
                MINIRE_ERROR("Mix_HaltChannel ({}): {}",
                             _channel, ::Mix_GetError());
            }
            pool->returnLease(_channel);
        }
    }

    AudioMixer::Player::Player(std::weak_ptr<Pool> && pool,
                               int channel) noexcept
        : _pool(std::move(pool))
        , _channel(channel)
    {}

    void AudioMixer::Player::play(formats::AudioClip const & audioClip,
                                  int const loops) const
    {
        MINIRE_INVARIANT(!_pool.expired(), "Pool is expired");
        if (::Mix_Chunk * chunk = AudioMixer::asChunk(audioClip); chunk)
        {
            assert(_channel >= 0);
            MINIRE_INVARIANT(_channel == ::Mix_PlayChannel(_channel, chunk, loops),
                             "failed to play AudioClip \"{}\" at channel {}: {}",
                             audioClip.filename(), _channel, ::Mix_GetError());
        }
    }

    float AudioMixer::Player::volume() const
    {
        assert(_channel >= 0);
        MINIRE_INVARIANT(!_pool.expired(), "Pool is expired");
        if (_volume < 0) _volume = ::Mix_Volume(_channel, -1);
        return volumeToFloat(_volume);
    }

    void AudioMixer::Player::setVolume(float volume)
    {
        assert(_channel >= 0);
        MINIRE_INVARIANT(!_pool.expired(), "Pool is expired");
        int const newVolume = volumeToInteger(volume);
        if (_volume < 0 || newVolume != _volume)
        {
            ::Mix_Volume(_channel, newVolume);
            _volume = newVolume;
        }
    }

    void AudioMixer::Player::stop()
    {
        assert(_channel >= 0);
        MINIRE_INVARIANT(!_pool.expired(), "Pool is expired");
        MINIRE_INVARIANT(-1 != ::Mix_HaltChannel(_channel),
                         "Mix_HaltChannel failed ({}): {}",
                         _channel, ::Mix_GetError());
    }

    void AudioMixer::Player::pause()
    {
        assert(_channel >= 0);
        MINIRE_INVARIANT(!_pool.expired(), "Pool is expired");
        ::Mix_Pause(_channel);
    }

    void AudioMixer::Player::resume()
    {
        assert(_channel >= 0);
        MINIRE_INVARIANT(!_pool.expired(), "Pool is expired");
        ::Mix_Resume(_channel);
    }

    bool AudioMixer::Player::paused() const
    {
        assert(_channel >= 0);
        MINIRE_INVARIANT(!_pool.expired(), "Pool is expired");
        return 1 == ::Mix_Paused(_channel);
    }

    // AudioMixer::Pool //

    AudioMixer::Pool::Pool(AudioMixer::Sptr && audioMixer,
                           int tag, std::vector<int> && channels,
                           AudioMixer::CtorToken)
        : _audioMixer(std::move(audioMixer))
        , _tag(tag)
        , _totalChannels(std::move(channels)) // won't throw
    {
        try
        {
            // may throw
            _freeChannels = _totalChannels;
            setChannelsTag(_tag);
        }
        catch(...)
        {
            returnResources();
            throw;
        }
    }

    AudioMixer::Pool::~Pool()
    {
        // NOTE: it may throw causing terminate(), accepting this behaviour
        //       because that could happen only in exceptionally broken
        //       conditions, so there is no point to continue program execution
        returnResources();
    }

    void AudioMixer::Pool::returnResources()
    {
        if (auto audioMixer = _audioMixer.lock(); audioMixer)
        {
            setChannelsTag(-1);
            audioMixer->deallocateTag(_tag);
            audioMixer->deallocateChannels(_totalChannels);
        }
    }

    std::unique_ptr<AudioMixer::Player> AudioMixer::Pool::lease()
    {
        MINIRE_INVARIANT(!_audioMixer.expired(), "AudioMixer is expired");
        if (!_freeChannels.empty())
        {
            int const channel = _freeChannels.back();
            _freeChannels.pop_back();

            auto deleter = [this](int channel) { _freeChannels.push_back(channel); };
            RaiiWrapper<int, decltype(deleter)> raiiChannel(channel, deleter);
            auto weakThis = weak_from_this(); // ensure it throws before release()
            return std::make_unique<Player>(Player(std::move(weakThis),
                                                   raiiChannel.release()));
        }
        return nullptr;
    }

    void AudioMixer::Pool::setChannelsTag(int tag)
    {
        for (int channel : _totalChannels)
        {
            MINIRE_INVARIANT(0 != ::Mix_GroupChannel(channel, tag),
                             "Mix_GroupChannel({}, {}) failed: {}",
                             channel, tag, ::Mix_GetError());
        }
    }

    void AudioMixer::Pool::returnLease(int channel)
    {
        assert(channel >= 0);
        assert(std::ranges::find(_totalChannels, channel) != _totalChannels.cend());
        _freeChannels.push_back(channel);
        assert(_freeChannels.size() <= _totalChannels.size());
    }

    bool AudioMixer::Pool::play(formats::AudioClip const & audioClip,
                                int const loops) const
    {
        MINIRE_INVARIANT(!_audioMixer.expired(), "AudioMixer is expired");
        if (::Mix_Chunk * chunk = AudioMixer::asChunk(audioClip); chunk)
        {
            if (int const channel = ::Mix_GroupAvailable(_tag);
                channel != -1)
            {
                return -1 != ::Mix_PlayChannel(channel, chunk, loops);
            }
        }
        return false;
    }

    void AudioMixer::Pool::stop()
    {
        MINIRE_INVARIANT(!_audioMixer.expired(), "AudioMixer is expired");
        ::Mix_HaltGroup(_tag);
    }

    // AudioMixer //

    AudioMixer::AudioMixer(models::MixerParams const & mixerParams)
        : _mixerParams(mixerParams)
    {
        // actually init the mixer
        if (::SDL_Init(SDL_INIT_AUDIO) != 0)
        {
            MINIRE_THROW("SDL_Init (SDL_INIT_AUDIO) failed: {}",
                         ::SDL_GetError());
        }

        if (0 != mixerParams._flags)
        {
            if ((::Mix_Init(mixerParams._flags) & mixerParams._flags) != mixerParams._flags)
            {
                MINIRE_THROW("Mix_Init failed: {}", ::Mix_GetError());
            }
        }

        if (::Mix_OpenAudio(mixerParams._frequency,
                            mixerParams._format,
                            mixerParams._channels,
                            mixerParams._chunksize) != 0)
        {
            ::Mix_Quit();
            MINIRE_THROW("Mix_OpenAudio failed: {}", ::Mix_GetError());
        }

        try
        {
            _channels = ::Mix_AllocateChannels(mixerParams._tracks);
            MINIRE_INFO("SDL_Mixer channels allocated: {}", _channels);

            _freeChannels.resize(_channels, 0);
            for(size_t i = 0; i < _channels; ++i)
            {
                _freeChannels[i] = i;
            }
        }
        catch(...)
        {
            ::Mix_CloseAudio();
            ::Mix_Quit();
            throw;
        }
    }

    AudioMixer::~AudioMixer()
    {
        ::Mix_CloseAudio();
        ::Mix_Quit();
    }

    AudioMixer::Pool::Sptr AudioMixer::makePool(size_t size, bool strictSize)
    {
        // allocate resource for the new Pool
        auto channelsDeleter = [this](std::vector<int> const & channels) { deallocateChannels(channels); };
        auto tagDeleter = [this](int tag) { deallocateTag(tag); };

        RaiiWrapper<std::vector<int>, decltype(channelsDeleter)> channels(allocateChannels(size),
                                                                          channelsDeleter);
        RaiiWrapper<int, decltype(tagDeleter)> tag(allocateTag(), tagDeleter);

        // check sanity
        MINIRE_INVARIANT(!strictSize || size == channels->size(),
                         "not enought non-reserved channels: "
                         "{} requested, but only {} available",
                         size, channels->size());

        // allocate the Pool
        auto sharedThis = shared_from_this(); // ensure it throws before release()s
        return std::make_shared<Pool>(std::move(sharedThis),
                                      tag.release(),
                                      std::move(channels.release()),
                                      CtorToken{});
    }

    AudioMixer::Pool::Sptr AudioMixer::makePool()
    {
        return makePool(_mixerParams._tracks, false);
    }

    std::vector<int> AudioMixer::allocateChannels(size_t size)
    {
        std::vector<int> result;
        result.reserve(size);
        if (size >= _freeChannels.size())
        {
            result = _freeChannels;
            _freeChannels.clear();
        }
        else
        {
            size_t offset = _freeChannels.size() - size;
            result.assign(_freeChannels.begin() + offset, _freeChannels.end());
            _freeChannels.resize(offset);
        }
        return result;
    }

    void AudioMixer::deallocateChannels(std::vector<int> const & channels)
    {
        _freeChannels.insert(_freeChannels.end(),
                             channels.begin(),
                             channels.end());
    }

    int AudioMixer::allocateTag()
    {
        if (!_freeTags.empty())
        {
            int result = _freeTags.back();
            _freeTags.pop_back();
            return result;
        }
        return _nextTag++;
    }

    void AudioMixer::deallocateTag(int freeTag)
    {
        if (freeTag >= 0)
        {
            _freeTags.push_back(freeTag);
        }
    }

    float AudioMixer::masterVolume() const
    {
        if (_masterVolume < 0) _masterVolume = ::Mix_MasterVolume(-1);
        return volumeToFloat(_masterVolume);
    }

    void AudioMixer::setMasterVolume(float volume)
    {
        if (int const newMasterVolume = volumeToInteger(volume);
            _masterVolume < 0 || _masterVolume != newMasterVolume)
        {
            ::Mix_MasterVolume(newMasterVolume);
            _masterVolume = newMasterVolume;
        }
    }

    void AudioMixer::stream(formats::AudioClip const & audioClip,
                            int const loops) const
    {
        if (::Mix_Music * music = audioClip.asMusic(); music)
        {
            MINIRE_INVARIANT(::Mix_PlayMusic(music, loops) != -1,
                             "Mix_PlayMusic failed for \"{}\": {}",
                             audioClip.filename(), ::Mix_GetError());
        }
    }

    void AudioMixer::stopStream()
    {
        ::Mix_HaltMusic();
    }

    float AudioMixer::streamVolume() const
    {
        if (_streamVolume < 0) _streamVolume = ::Mix_VolumeMusic(-1);
        return volumeToFloat(_streamVolume);
    }

    void AudioMixer::setStreamVolume(float volume)
    {
        if (int const newStreamVolume = volumeToInteger(volume);
            _streamVolume < 0 || _streamVolume != newStreamVolume)
        {
            ::Mix_VolumeMusic(newStreamVolume);
            _streamVolume = newStreamVolume;
        }
    }

    bool AudioMixer::isPlayingStream() const
    {
        return 1 == ::Mix_PlayingMusic();
    }

    // Just a wrapper to help keeping AudioClip internal private
    ::Mix_Chunk * AudioMixer::asChunk(formats::AudioClip const & audioClip)
    {
        return audioClip.asChunk();
    }
}