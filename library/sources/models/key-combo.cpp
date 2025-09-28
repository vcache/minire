#include <minire/models/key-combo.hpp>

#include <minire/errors.hpp>
 
#include <SDL2/SDL_keyboard.h>

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <utility>
#include <vector>

namespace minire::models
{
    // see https://wiki.libsdl.org/SDL_Keymod
    namespace
    {
        static constexpr const uint16_t kAllowedModMask = 
            KMOD_LSHIFT | KMOD_RSHIFT |
            KMOD_LCTRL  | KMOD_RCTRL  |
            KMOD_LALT   | KMOD_RALT   |
            KMOD_LGUI   | KMOD_RGUI   |
            KMOD_CTRL   | KMOD_SHIFT  |
            KMOD_ALT    | KMOD_GUI;

        static const std::vector<std::pair<std::string, uint16_t>> kStringToModCombined
        {
            {"Ctrl", KMOD_CTRL},
            {"Shift", KMOD_SHIFT},
            {"Alt", KMOD_ALT},
            {"Gui", KMOD_GUI},
        };

        static const std::vector<std::pair<std::string, uint16_t>> kStringToModSingles
        {
            {"Left Shift", KMOD_LSHIFT},
            {"Right Shift", KMOD_RSHIFT},
            {"Left Ctrl", KMOD_LCTRL},
            {"Right Ctrl", KMOD_RCTRL},
            {"Left Alt", KMOD_LALT},
            {"Right Alt", KMOD_RALT},
            {"Left Gui", KMOD_LGUI},
            {"Right Gui", KMOD_RGUI},
        };

        struct StringHashAnyCase
        {
            size_t operator()(std::string const & input) const noexcept
            {
                std::hash<char> h;
                size_t result = 0;
                for (char c : input) c += h(std::toupper(c));
                return result;                
            }
        };

        struct StringPredAnyCase
        {
            bool operator()(std::string const & first,
                            std::string const & second) const noexcept
            {
                return first.size() == second.size()
                    && std::equal(first.cbegin(), first.cend(), second.cbegin(),
                                  [](char a, char b){ return std::toupper(a) == std::toupper(b); });
            }
        };

        static auto const & gKeycodesMap()
        {
            static const std::unordered_map<std::string,
                                            ::SDL_Scancode,
                                            StringHashAnyCase,
                                            StringPredAnyCase> kKeycodesMap
            {
                {"A", SDL_SCANCODE_A},
                {"B", SDL_SCANCODE_B},
                {"C", SDL_SCANCODE_C},
                {"D", SDL_SCANCODE_D},
                {"E", SDL_SCANCODE_E},
                {"F", SDL_SCANCODE_F},
                {"G", SDL_SCANCODE_G},
                {"H", SDL_SCANCODE_H},
                {"I", SDL_SCANCODE_I},
                {"J", SDL_SCANCODE_J},
                {"K", SDL_SCANCODE_K},
                {"L", SDL_SCANCODE_L},
                {"M", SDL_SCANCODE_M},
                {"N", SDL_SCANCODE_N},
                {"O", SDL_SCANCODE_O},
                {"P", SDL_SCANCODE_P},
                {"Q", SDL_SCANCODE_Q},
                {"R", SDL_SCANCODE_R},
                {"S", SDL_SCANCODE_S},
                {"T", SDL_SCANCODE_T},
                {"U", SDL_SCANCODE_U},
                {"V", SDL_SCANCODE_V},
                {"W", SDL_SCANCODE_W},
                {"X", SDL_SCANCODE_X},
                {"Y", SDL_SCANCODE_Y},
                {"Z", SDL_SCANCODE_Z},
                {"1", SDL_SCANCODE_1},
                {"2", SDL_SCANCODE_2},
                {"3", SDL_SCANCODE_3},
                {"4", SDL_SCANCODE_4},
                {"5", SDL_SCANCODE_5},
                {"6", SDL_SCANCODE_6},
                {"7", SDL_SCANCODE_7},
                {"8", SDL_SCANCODE_8},
                {"9", SDL_SCANCODE_9},
                {"0", SDL_SCANCODE_0},
                {"ENTER", SDL_SCANCODE_RETURN},
                {"ESCAPE", SDL_SCANCODE_ESCAPE},
                {"BACKSPACE", SDL_SCANCODE_BACKSPACE},
                {"TAB", SDL_SCANCODE_TAB},
                {"SPACE", SDL_SCANCODE_SPACE},
                {"MINUS", SDL_SCANCODE_MINUS},
                {"EQUALS", SDL_SCANCODE_EQUALS},
                {"LEFTBRACKET", SDL_SCANCODE_LEFTBRACKET},
                {"RIGHTBRACKET", SDL_SCANCODE_RIGHTBRACKET},
                {"BACKSLASH", SDL_SCANCODE_BACKSLASH},
                {"NONUSHASH", SDL_SCANCODE_NONUSHASH},
                {"SEMICOLON", SDL_SCANCODE_SEMICOLON},
                {"APOSTROPHE", SDL_SCANCODE_APOSTROPHE},
                {"GRAVE", SDL_SCANCODE_GRAVE},
                {"COMMA", SDL_SCANCODE_COMMA},
                {"PERIOD", SDL_SCANCODE_PERIOD},
                {"SLASH", SDL_SCANCODE_SLASH},
                {"CAPSLOCK", SDL_SCANCODE_CAPSLOCK},
                {"F1", SDL_SCANCODE_F1},
                {"F2", SDL_SCANCODE_F2},
                {"F3", SDL_SCANCODE_F3},
                {"F4", SDL_SCANCODE_F4},
                {"F5", SDL_SCANCODE_F5},
                {"F6", SDL_SCANCODE_F6},
                {"F7", SDL_SCANCODE_F7},
                {"F8", SDL_SCANCODE_F8},
                {"F9", SDL_SCANCODE_F9},
                {"F10", SDL_SCANCODE_F10},
                {"F11", SDL_SCANCODE_F11},
                {"F12", SDL_SCANCODE_F12},
                {"PRINTSCREEN", SDL_SCANCODE_PRINTSCREEN},
                {"SCROLLLOCK", SDL_SCANCODE_SCROLLLOCK},
                {"PAUSE", SDL_SCANCODE_PAUSE},
                {"INSERT", SDL_SCANCODE_INSERT},
                {"HOME", SDL_SCANCODE_HOME},
                {"PAGEUP", SDL_SCANCODE_PAGEUP},
                {"DELETE", SDL_SCANCODE_DELETE},
                {"END", SDL_SCANCODE_END},
                {"PAGEDOWN", SDL_SCANCODE_PAGEDOWN},
                {"RIGHT", SDL_SCANCODE_RIGHT},
                {"LEFT", SDL_SCANCODE_LEFT},
                {"DOWN", SDL_SCANCODE_DOWN},
                {"UP", SDL_SCANCODE_UP},
                {"NUMLOCKCLEAR", SDL_SCANCODE_NUMLOCKCLEAR},
                {"KP_DIVIDE", SDL_SCANCODE_KP_DIVIDE},
                {"KP_MULTIPLY", SDL_SCANCODE_KP_MULTIPLY},
                {"KP_MINUS", SDL_SCANCODE_KP_MINUS},
                {"KP_PLUS", SDL_SCANCODE_KP_PLUS},
                {"KP_ENTER", SDL_SCANCODE_KP_ENTER},
                {"KP_1", SDL_SCANCODE_KP_1},
                {"KP_2", SDL_SCANCODE_KP_2},
                {"KP_3", SDL_SCANCODE_KP_3},
                {"KP_4", SDL_SCANCODE_KP_4},
                {"KP_5", SDL_SCANCODE_KP_5},
                {"KP_6", SDL_SCANCODE_KP_6},
                {"KP_7", SDL_SCANCODE_KP_7},
                {"KP_8", SDL_SCANCODE_KP_8},
                {"KP_9", SDL_SCANCODE_KP_9},
                {"KP_0", SDL_SCANCODE_KP_0},
                {"KP_PERIOD", SDL_SCANCODE_KP_PERIOD},
                {"NONUSBACKSLASH", SDL_SCANCODE_NONUSBACKSLASH},
                {"APPLICATION", SDL_SCANCODE_APPLICATION},
                {"POWER", SDL_SCANCODE_POWER},
                {"KP_EQUALS", SDL_SCANCODE_KP_EQUALS},
                {"F13", SDL_SCANCODE_F13},
                {"F14", SDL_SCANCODE_F14},
                {"F15", SDL_SCANCODE_F15},
                {"F16", SDL_SCANCODE_F16},
                {"F17", SDL_SCANCODE_F17},
                {"F18", SDL_SCANCODE_F18},
                {"F19", SDL_SCANCODE_F19},
                {"F20", SDL_SCANCODE_F20},
                {"F21", SDL_SCANCODE_F21},
                {"F22", SDL_SCANCODE_F22},
                {"F23", SDL_SCANCODE_F23},
                {"F24", SDL_SCANCODE_F24},
                {"EXECUTE", SDL_SCANCODE_EXECUTE},
                {"HELP", SDL_SCANCODE_HELP},
                {"MENU", SDL_SCANCODE_MENU},
                {"SELECT", SDL_SCANCODE_SELECT},
                {"STOP", SDL_SCANCODE_STOP},
                {"AGAIN", SDL_SCANCODE_AGAIN},
                {"UNDO", SDL_SCANCODE_UNDO},
                {"CUT", SDL_SCANCODE_CUT},
                {"COPY", SDL_SCANCODE_COPY},
                {"PASTE", SDL_SCANCODE_PASTE},
                {"FIND", SDL_SCANCODE_FIND},
                {"MUTE", SDL_SCANCODE_MUTE},
                {"VOLUMEUP", SDL_SCANCODE_VOLUMEUP},
                {"VOLUMEDOWN", SDL_SCANCODE_VOLUMEDOWN},
                {"KP_COMMA", SDL_SCANCODE_KP_COMMA},
                {"KP_EQUALSAS400", SDL_SCANCODE_KP_EQUALSAS400},
                {"INTERNATIONAL1", SDL_SCANCODE_INTERNATIONAL1},
                {"INTERNATIONAL2", SDL_SCANCODE_INTERNATIONAL2},
                {"INTERNATIONAL3", SDL_SCANCODE_INTERNATIONAL3},
                {"INTERNATIONAL4", SDL_SCANCODE_INTERNATIONAL4},
                {"INTERNATIONAL5", SDL_SCANCODE_INTERNATIONAL5},
                {"INTERNATIONAL6", SDL_SCANCODE_INTERNATIONAL6},
                {"INTERNATIONAL7", SDL_SCANCODE_INTERNATIONAL7},
                {"INTERNATIONAL8", SDL_SCANCODE_INTERNATIONAL8},
                {"INTERNATIONAL9", SDL_SCANCODE_INTERNATIONAL9},
                {"LANG1", SDL_SCANCODE_LANG1},
                {"LANG2", SDL_SCANCODE_LANG2},
                {"LANG3", SDL_SCANCODE_LANG3},
                {"LANG4", SDL_SCANCODE_LANG4},
                {"LANG5", SDL_SCANCODE_LANG5},
                {"LANG6", SDL_SCANCODE_LANG6},
                {"LANG7", SDL_SCANCODE_LANG7},
                {"LANG8", SDL_SCANCODE_LANG8},
                {"LANG9", SDL_SCANCODE_LANG9},
                {"ALTERASE", SDL_SCANCODE_ALTERASE},
                {"SYSREQ", SDL_SCANCODE_SYSREQ},
                {"CANCEL", SDL_SCANCODE_CANCEL},
                {"CLEAR", SDL_SCANCODE_CLEAR},
                {"PRIOR", SDL_SCANCODE_PRIOR},
                {"RETURN2", SDL_SCANCODE_RETURN2},
                {"SEPARATOR", SDL_SCANCODE_SEPARATOR},
                {"OUT", SDL_SCANCODE_OUT},
                {"OPER", SDL_SCANCODE_OPER},
                {"CLEARAGAIN", SDL_SCANCODE_CLEARAGAIN},
                {"CRSEL", SDL_SCANCODE_CRSEL},
                {"EXSEL", SDL_SCANCODE_EXSEL},
                {"KP_00", SDL_SCANCODE_KP_00},
                {"KP_000", SDL_SCANCODE_KP_000},
                {"THOUSANDSSEPARATOR", SDL_SCANCODE_THOUSANDSSEPARATOR},
                {"DECIMALSEPARATOR", SDL_SCANCODE_DECIMALSEPARATOR},
                {"CURRENCYUNIT", SDL_SCANCODE_CURRENCYUNIT},
                {"CURRENCYSUBUNIT", SDL_SCANCODE_CURRENCYSUBUNIT},
                {"KP_LEFTPAREN", SDL_SCANCODE_KP_LEFTPAREN},
                {"KP_RIGHTPAREN", SDL_SCANCODE_KP_RIGHTPAREN},
                {"KP_LEFTBRACE", SDL_SCANCODE_KP_LEFTBRACE},
                {"KP_RIGHTBRACE", SDL_SCANCODE_KP_RIGHTBRACE},
                {"KP_TAB", SDL_SCANCODE_KP_TAB},
                {"KP_BACKSPACE", SDL_SCANCODE_KP_BACKSPACE},
                {"KP_A", SDL_SCANCODE_KP_A},
                {"KP_B", SDL_SCANCODE_KP_B},
                {"KP_C", SDL_SCANCODE_KP_C},
                {"KP_D", SDL_SCANCODE_KP_D},
                {"KP_E", SDL_SCANCODE_KP_E},
                {"KP_F", SDL_SCANCODE_KP_F},
                {"KP_XOR", SDL_SCANCODE_KP_XOR},
                {"KP_POWER", SDL_SCANCODE_KP_POWER},
                {"KP_PERCENT", SDL_SCANCODE_KP_PERCENT},
                {"KP_LESS", SDL_SCANCODE_KP_LESS},
                {"KP_GREATER", SDL_SCANCODE_KP_GREATER},
                {"KP_AMPERSAND", SDL_SCANCODE_KP_AMPERSAND},
                {"KP_DBLAMPERSAND", SDL_SCANCODE_KP_DBLAMPERSAND},
                {"KP_VERTICALBAR", SDL_SCANCODE_KP_VERTICALBAR},
                {"KP_DBLVERTICALBAR", SDL_SCANCODE_KP_DBLVERTICALBAR},
                {"KP_COLON", SDL_SCANCODE_KP_COLON},
                {"KP_HASH", SDL_SCANCODE_KP_HASH},
                {"KP_SPACE", SDL_SCANCODE_KP_SPACE},
                {"KP_AT", SDL_SCANCODE_KP_AT},
                {"KP_EXCLAM", SDL_SCANCODE_KP_EXCLAM},
                {"KP_MEMSTORE", SDL_SCANCODE_KP_MEMSTORE},
                {"KP_MEMRECALL", SDL_SCANCODE_KP_MEMRECALL},
                {"KP_MEMCLEAR", SDL_SCANCODE_KP_MEMCLEAR},
                {"KP_MEMADD", SDL_SCANCODE_KP_MEMADD},
                {"KP_MEMSUBTRACT", SDL_SCANCODE_KP_MEMSUBTRACT},
                {"KP_MEMMULTIPLY", SDL_SCANCODE_KP_MEMMULTIPLY},
                {"KP_MEMDIVIDE", SDL_SCANCODE_KP_MEMDIVIDE},
                {"KP_PLUSMINUS", SDL_SCANCODE_KP_PLUSMINUS},
                {"KP_CLEAR", SDL_SCANCODE_KP_CLEAR},
                {"KP_CLEARENTRY", SDL_SCANCODE_KP_CLEARENTRY},
                {"KP_BINARY", SDL_SCANCODE_KP_BINARY},
                {"KP_OCTAL", SDL_SCANCODE_KP_OCTAL},
                {"KP_DECIMAL", SDL_SCANCODE_KP_DECIMAL},
                {"KP_HEXADECIMAL", SDL_SCANCODE_KP_HEXADECIMAL},
                {"Left CTRL", SDL_SCANCODE_LCTRL},
                {"Left SHIFT", SDL_SCANCODE_LSHIFT},
                {"Left ALT", SDL_SCANCODE_LALT},
                {"Left GUI", SDL_SCANCODE_LGUI},
                {"Right CTRL", SDL_SCANCODE_RCTRL},
                {"Right SHIFT", SDL_SCANCODE_RSHIFT},
                {"Right ALT", SDL_SCANCODE_RALT},
                {"Right GUI", SDL_SCANCODE_RGUI},
                {"MODE", SDL_SCANCODE_MODE},
                {"AUDIONEXT", SDL_SCANCODE_AUDIONEXT},
                {"AUDIOPREV", SDL_SCANCODE_AUDIOPREV},
                {"AUDIOSTOP", SDL_SCANCODE_AUDIOSTOP},
                {"AUDIOPLAY", SDL_SCANCODE_AUDIOPLAY},
                {"AUDIOMUTE", SDL_SCANCODE_AUDIOMUTE},
                {"MEDIASELECT", SDL_SCANCODE_MEDIASELECT},
                {"WWW", SDL_SCANCODE_WWW},
                {"MAIL", SDL_SCANCODE_MAIL},
                {"CALCULATOR", SDL_SCANCODE_CALCULATOR},
                {"COMPUTER", SDL_SCANCODE_COMPUTER},
                {"AC_SEARCH", SDL_SCANCODE_AC_SEARCH},
                {"AC_HOME", SDL_SCANCODE_AC_HOME},
                {"AC_BACK", SDL_SCANCODE_AC_BACK},
                {"AC_FORWARD", SDL_SCANCODE_AC_FORWARD},
                {"AC_STOP", SDL_SCANCODE_AC_STOP},
                {"AC_REFRESH", SDL_SCANCODE_AC_REFRESH},
                {"AC_BOOKMARKS", SDL_SCANCODE_AC_BOOKMARKS},
                {"BRIGHTNESSDOWN", SDL_SCANCODE_BRIGHTNESSDOWN},
                {"BRIGHTNESSUP", SDL_SCANCODE_BRIGHTNESSUP},
                {"DISPLAYSWITCH", SDL_SCANCODE_DISPLAYSWITCH},
                {"KBDILLUMTOGGLE", SDL_SCANCODE_KBDILLUMTOGGLE},
                {"KBDILLUMDOWN", SDL_SCANCODE_KBDILLUMDOWN},
                {"KBDILLUMUP", SDL_SCANCODE_KBDILLUMUP},
                {"EJECT", SDL_SCANCODE_EJECT},
                {"SLEEP", SDL_SCANCODE_SLEEP},
                {"APP1", SDL_SCANCODE_APP1},
                {"APP2", SDL_SCANCODE_APP2},
                {"AUDIOREWIND", SDL_SCANCODE_AUDIOREWIND},
                {"AUDIOFASTFORWARD", SDL_SCANCODE_AUDIOFASTFORWARD}               
            };
            return kKeycodesMap;
        }

        static auto const & gKeycodesMapRev()
        {
            static std::unordered_map<::SDL_Scancode, std::string> kKeycodesMapRev;
            if (kKeycodesMapRev.empty())
            {
                for(auto const & i : gKeycodesMap())
                {
                    kKeycodesMapRev.emplace(i.second, i.first);
                }
            }
            return kKeycodesMapRev;
        }

        size_t measureSeparator(std::string const & str, size_t pos)
        {
            size_t begin = pos;
            size_t plusCount = 0;
            while(pos < str.size() &&
                  (str[pos] == '+' || ::isspace(str[pos])))
            {
                if (str[pos] == '+')
                {
                    ++plusCount;
                    if (plusCount > 1) break;
                }
                ++pos;
            }
            return pos - begin;
        }

        bool isStartsWith(std::string const & input,
                          std::string const & prefix)
        {
            if (input.size() < prefix.size()) return false;
            return std::equal(prefix.cbegin(), prefix.cend(),
                              input.cbegin(), [](char a, char b)
                {
                    return std::toupper(a) == std::toupper(b);
                });
        }

        std::optional<uint16_t> consumeModifier(std::string & input)
        {
            if (input.empty()) return std::nullopt;

            for(auto const & [str, code] : kStringToModCombined)
            {
                if (isStartsWith(input, str))
                {
                    input.erase(0, str.size() + measureSeparator(input, str.size()));
                    return code;
                }
            }

            for(auto const & [str, code] : kStringToModSingles)
            {
                if (isStartsWith(input, str))
                {
                    input.erase(0, str.size() + measureSeparator(input, str.size()));
                    return code;
                }
            }

            return std::nullopt;
        }
    }

    std::string KeyCombo::normalize(std::string const & input)
    {
        std::string result;
        result.reserve(input.size());
        auto it = input.cbegin(), end = input.cend();
        while(it != end && *it == ' ') ++it;
        while(it != end)
        {
            if (!::isspace(*it))
            {
                result += *it;
                ++it;
            }
            else
            {
                while(it != end && ::isspace(*it)) ++it;
                if (it != end) result += ' ';
            }
        }
        return result;
    }

    KeyCombo::KeyCombo(::SDL_Scancode scancode, uint16_t mods)
        : _scancode(scancode)
        , _mods(mods)
    {
        if (gKeycodesMapRev().find(_scancode) == gKeycodesMapRev().cend())
        {
            MINIRE_THROW("unknown key code: {:x} ({})",
                         static_cast<int>(_scancode),
                         ::SDL_GetScancodeName(_scancode));
        }
    }

    KeyCombo::KeyCombo(std::string const & in)
        : _scancode(SDL_SCANCODE_UNKNOWN)
        , _mods(0)
    {
        std::string nomalized = normalize(in);
        while(!nomalized.empty())
        {
            if (auto it = gKeycodesMap().find(nomalized);
                it != gKeycodesMap().cend())
            {
                _scancode = it->second;
                break;
            }
            std::optional<uint16_t> modifier = consumeModifier(nomalized);
            MINIRE_INVARIANT(modifier, "cannot parse KeyCombo from '{}' in '{}'",
                             nomalized, in);
            _mods |= *modifier;
        }
        MINIRE_INVARIANT(_scancode != SDL_SCANCODE_UNKNOWN, "no KeyCombo '{}'", in);
    }

    std::string KeyCombo::toString() const
    {
        std::string result;
        int mods = _mods;
        if (mods)
        {
            for(auto const & i : kStringToModCombined)
            {
                if (i.second == (mods & i.second))
                {
                    result += i.first + " + ";
                    mods &= ~(i.second);
                }
            }

            for(auto const & i : kStringToModSingles)
            {
                if (mods & i.second)
                {
                    result += i.first + " + ";
                    mods &= ~(i.second);
                }
            }

            MINIRE_INVARIANT(!mods, "some mods are left: {}; inital {}", mods, _mods);
        }

        auto it = gKeycodesMapRev().find(_scancode);
        MINIRE_INVARIANT(it != gKeycodesMapRev().cend(),
                         "unknown key code: {:x}", static_cast<int>(_scancode));
        result += it->second;

        return result;
    }

    namespace
    {
        void multiplyMods(std::vector<uint16_t> & result,
                          uint16_t a, uint16_t b)
        {
            size_t origSize = result.size();
            assert(origSize != 0);

            for(uint16_t & i : result) i |= a;

            for(size_t i = 0; i < origSize; ++i)
            {
                result.push_back((result[i] & (~a)) | b);
            }
        }

        // TODO: this could be precalculated
        std::vector<uint16_t> inflateMods(uint16_t original)
        {
            bool haveCtrl = (original & KMOD_CTRL) == KMOD_CTRL;
            bool haveShift = (original & KMOD_SHIFT) == KMOD_SHIFT;
            bool haveAlt = (original & KMOD_ALT) == KMOD_ALT;
            bool haveGui = (original & KMOD_GUI) == KMOD_GUI;

            if (haveCtrl) original &= ~KMOD_CTRL;
            if (haveShift) original &= ~KMOD_SHIFT;
            if (haveAlt) original &= ~KMOD_ALT;
            if (haveGui) original &= ~KMOD_GUI;

            std::vector<uint16_t> result{original};

            if (haveCtrl) multiplyMods(result,  KMOD_LCTRL,  KMOD_RCTRL);
            if (haveShift) multiplyMods(result, KMOD_LSHIFT, KMOD_RSHIFT);
            if (haveAlt) multiplyMods(result,   KMOD_LALT,   KMOD_RALT);
            if (haveGui) multiplyMods(result,   KMOD_LGUI,   KMOD_RGUI);

            return result;
        }
    }

    std::vector<KeyCombo>
    KeyCombo::expandCombinedMods(::SDL_Scancode key, uint16_t mod)
    {
        MINIRE_INVARIANT((mod & kAllowedModMask) == mod,
                         "some mod flags not supported: {:x}", mod);

        std::vector<KeyCombo> result;
        if (0 == mod)
        {
            result.emplace_back(key, mod);
        }
        else
        {
            for(uint16_t m : inflateMods(mod))
            {
                result.emplace_back(key, m);
            }
        }
        return result;
    }

    std::vector<KeyCombo>
    KeyCombo::expandCombinedMods(KeyCombo const & keyCombo)
    {
        return expandCombinedMods(keyCombo.scancode(), keyCombo.mods());
    }
}