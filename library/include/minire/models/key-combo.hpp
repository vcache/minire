#pragma once

#include <boost/container_hash/hash.hpp> // for hash_combine
#include <fmt/format.h>
#include <SDL_keycode.h> // for ::SDL_Scancode and KMOD_

#include <functional> // for std::hash
#include <vector>

namespace minire::models
{
    class KeyCombo
    {
    public:
        explicit KeyCombo(std::string const &);

        explicit KeyCombo(::SDL_Scancode key, uint16_t mods = 0);

        std::string toString() const;

        bool operator==(KeyCombo const & t) const
        {
            return _scancode == t._scancode && _mods == t._mods;
        }

        ::SDL_Scancode scancode() const noexcept { return _scancode; }

        uint16_t mods() const noexcept { return _mods; }

        static std::string normalize(std::string const &);

        static std::vector<KeyCombo> expandCombinedMods(::SDL_Scancode key,
                                                        uint16_t mod);

        static std::vector<KeyCombo> expandCombinedMods(KeyCombo const &);

    private:
        ::SDL_Scancode _scancode;
        uint16_t       _mods;
    };
}

namespace std
{
    template<>
    struct hash<::minire::models::KeyCombo>
    {
        size_t operator()(::minire::models::KeyCombo const & in) const noexcept
        {
            size_t const first = hash<::SDL_Scancode>()(in.scancode());
            size_t const second = hash<uint16_t>()(in.mods());

            size_t result = 0;
            boost::hash_combine(result, first);
            boost::hash_combine(result, second);
            return result;
        }
    };
}

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_same_v<T, minire::models::KeyCombo>, char>>
    : fmt::formatter<std::string>
{
    template <typename FormatCtx>
    auto format(T const & value, FormatCtx & ctx) const
    {
        return fmt::formatter<std::string>::format(value.toString(), ctx);
    }
};
