#pragma once

#include <memory>
#include <vector>

#include <minire/models/mixer-params.hpp>

namespace minire::formats { class AudioClip; }
namespace minire::models { class MixerParams; }

namespace minire::sdl
{
    // On loops: AudioClip will be player "loops + 1" times if loops >= 0,
    // or infinitely if loops == -1.
    //
    // Make sure AudioMixer won't outlive Player or Pool.
    //
    // This class cached some of values of SDL_Mixer's global state, therefore,
    // changes them manually (via SDL_mixer API, or second instance of AudioMixer)
    // will break the AudioMixer and should be avoided.
    class AudioMixer
        : public std::enable_shared_from_this<AudioMixer>
    {
        AudioMixer(AudioMixer const &) = delete;
        AudioMixer& operator=(AudioMixer const &) = delete;
        AudioMixer(AudioMixer &&) = delete;
        AudioMixer& operator=(AudioMixer &&) = delete;

        // A helper to make public ctor of Pool inaccessible by external user.
        // This class must be always private.
        struct CtorToken {};

    public:
        explicit AudioMixer(models::MixerParams const &);
        ~AudioMixer();

        using Sptr = std::shared_ptr<AudioMixer>;
        using Wptr = std::weak_ptr<AudioMixer>;

    public:
        // Volumes are values between 0.0f and 1.0f
        float masterVolume() const;
        void setMasterVolume(float);

    public:
        // Streaming is a dedicated track for a long-playing
        // recordings, such as music or radio (i.e. will be decoded on-fly).
        // See SDL's Mix_Music for details.

        void stream(formats::AudioClip const &,
                    int const loops = 0) const;
        float streamVolume() const;
        void setStreamVolume(float);
        bool isPlayingStream() const;

    public:
        class Pool;

        // A Player is a reservation of a track/channel of a Pool.
        // AudioClip will be played as SDL's Mix_Chunk (i.e. fully decoded).
        class Player
        {
            Player(Player const &) = delete;
            Player& operator=(Player const &) = delete;

        public:
            Player(Player && other) noexcept;
            Player& operator=(Player && other) noexcept;

            ~Player();

        public:
            void play(formats::AudioClip const &,
                      int const loops = 0) const;

            // NOTE: Player::setVolume can be called very often
            //       even when the volume didn't change.
            //       Since a Player is an exclusive user of a Track,
            //       it can safely cache volume's value.

            float volume() const;
            void setVolume(float);

            void stop();
            void pause();
            void resume();
            bool paused() const;

        private:
            // NOTE: passing std::weak_ptr by r-value ref to prevent exceptions
            explicit Player(std::weak_ptr<Pool> && pool,
                            int channel) noexcept;

        private:
            std::weak_ptr<Pool> _pool;
            int                 _channel = -1;
            mutable int         _volume = -1;

            friend class Pool;
        };

        // A collection of player (as in SDL's Channel Group)
        class Pool
            : public std::enable_shared_from_this<Pool>
        {
            Pool(Pool const &) = delete;
            Pool& operator=(Pool const &) = delete;
            Pool(Pool && other) = delete;
            Pool& operator=(Pool && other) = delete;

        public:
            using Sptr = std::shared_ptr<Pool>;
            using Wptr = std::weak_ptr<Pool>;

            ~Pool();

        public:
            // return non-busy Track from a Pool
            std::unique_ptr<Player> lease();

            size_t size() const { return _totalChannels.size(); }

            // plays AudioClip asynchronously if there is any idle Track,
            // no Player will be created, the playback may be interrupted
            // at any moment. The call will not fail even if the playback
            // didn't start.
            // It can playback on a leased non-busy Player, but Player user
            // will have priority (will interrupt if needs).
            bool play(formats::AudioClip const &,
                      int const loops = 0) const;

            // Interrupt all current playbacks
            void stop();

        public:
            // NOTE: passing AudioMixer::Sptr by r-value ref to prevent exceptions
            explicit Pool(AudioMixer::Sptr && audioMixer,
                          int tag, std::vector<int> && channels,
                          CtorToken);

        private:
            void returnLease(int channel);
            void returnResources();
            void setChannelsTag(int tag);

        private:
            AudioMixer::Wptr _audioMixer;
            int              _tag = 0;
            std::vector<int> _totalChannels;
            std::vector<int> _freeChannels;

            friend class AudioMixer;
            friend class Player;
        };

    public:
        size_t totalTracks() const { return _channels; }
        size_t availableTracks() const { return _freeChannels.size(); }

        Pool::Sptr makePool(size_t size, bool strictSize = true);
        Pool::Sptr makePool(); // all empty Tracks

    private:
        int allocateTag();
        void deallocateTag(int);

        std::vector<int> allocateChannels(size_t size);
        void deallocateChannels(std::vector<int> const & channels);

        static ::Mix_Chunk * asChunk(formats::AudioClip const &);

    private:
        models::MixerParams const _mixerParams;
        std::vector<int>          _freeChannels;
        size_t                    _channels = 0;
        std::vector<int>          _freeTags;
        int                       _nextTag = 0;

        mutable int               _masterVolume = -1;
        mutable int               _streamVolume = -1;

        friend class Pool;
    };
}
