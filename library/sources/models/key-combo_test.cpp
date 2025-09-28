#include <minire/models/key-combo.hpp>

#include <minire/errors.hpp>

#include <gtest/gtest.h>

#include <optional>

namespace minire::models::tests
{
    struct KeyComboCase
    {
        std::string             _string;
        std::optional<KeyCombo> _keyCombo;
    };

    class KeyComboParser
        : public testing::TestWithParam<KeyComboCase>
    {};

    TEST_P(KeyComboParser, Parsing)
    {
        KeyComboCase const & param = GetParam();
        if (param._keyCombo)
        {
            KeyCombo parsed(param._string);
            EXPECT_EQ(parsed, *param._keyCombo);
        }
        else
        {
            EXPECT_THROW(std::make_unique<KeyCombo>(param._string),
                         Exception);
        }
    }

    class KeyComboEmitter
        : public testing::TestWithParam<KeyComboCase>
    {};

    TEST_P(KeyComboEmitter, Parsing)
    {
        KeyComboCase const & param = GetParam();
        if (param._keyCombo)
        {
            EXPECT_STRCASEEQ(KeyCombo::normalize(param._string).c_str(),
                             param._keyCombo->toString().c_str());
        }
    }

    static const auto kCases = testing::Values
    (
        KeyComboCase{"", std::nullopt}
       ,KeyComboCase{"Enter", KeyCombo(SDL_SCANCODE_RETURN)}
       ,KeyComboCase{"Ctrl + Enter", KeyCombo(SDL_SCANCODE_RETURN, KMOD_CTRL)}
       ,KeyComboCase{"Left Ctrl + A", KeyCombo(SDL_SCANCODE_A, KMOD_LCTRL)}
       ,KeyComboCase{"Left Ctrl", KeyCombo(SDL_SCANCODE_LCTRL)}
       ,KeyComboCase{"  gui  +  \tRight   ALT  +  KP_HASH\n  ",
                     KeyCombo(SDL_SCANCODE_KP_HASH, KMOD_GUI | KMOD_RALT)}
       ,KeyComboCase{"Left Alt + KP_PLUS", KeyCombo(SDL_SCANCODE_KP_PLUS, KMOD_LALT)}
       ,KeyComboCase{"Right Ctrl + Left Alt + KP_PLUSMINUS",
                       KeyCombo(SDL_SCANCODE_KP_PLUSMINUS, KMOD_LALT | KMOD_RCTRL)}
    );

    INSTANTIATE_TEST_SUITE_P(KeyComboParserGroup,
                             KeyComboParser,
                             kCases);

    INSTANTIATE_TEST_SUITE_P(KeyComboEmitterGroup,
                             KeyComboEmitter,
                             kCases);
}

namespace minire::models::tests
{
    struct KeyComboInflationCase
    {
        KeyCombo              _source;
        std::vector<KeyCombo> _expected;
    };

    class KeyComboInflation
        : public testing::TestWithParam<KeyComboInflationCase>
    {};

    TEST_P(KeyComboInflation, Basic)
    {
        KeyComboInflationCase const & param = GetParam();
        auto result = KeyCombo::expandCombinedMods(param._source.scancode(),
                                                   param._source.mods());
        EXPECT_EQ(result, param._expected);
    }

    INSTANTIATE_TEST_SUITE_P(
        KeyComboInflationGroup,
        KeyComboInflation,
        testing::Values(
            KeyComboInflationCase
            {
                KeyCombo(SDL_SCANCODE_RETURN),
                {KeyCombo(SDL_SCANCODE_RETURN)}
            }
           ,KeyComboInflationCase
            {
                KeyCombo(SDL_SCANCODE_RETURN, KMOD_LCTRL),
                {KeyCombo(SDL_SCANCODE_RETURN, KMOD_LCTRL)}
            }
           ,KeyComboInflationCase
            {
                KeyCombo(SDL_SCANCODE_RETURN, KMOD_CTRL),
                {
                    KeyCombo(SDL_SCANCODE_RETURN, KMOD_LCTRL),
                    KeyCombo(SDL_SCANCODE_RETURN, KMOD_RCTRL)
                }
            }
           ,KeyComboInflationCase
            {
                KeyCombo(SDL_SCANCODE_DELETE, KMOD_CTRL | KMOD_ALT),
                {
                    KeyCombo(SDL_SCANCODE_DELETE, KMOD_LCTRL | KMOD_LALT),
                    KeyCombo(SDL_SCANCODE_DELETE, KMOD_RCTRL | KMOD_LALT),
                    KeyCombo(SDL_SCANCODE_DELETE, KMOD_LCTRL | KMOD_RALT),
                    KeyCombo(SDL_SCANCODE_DELETE, KMOD_RCTRL | KMOD_RALT),
                }
            }
           ,KeyComboInflationCase
            {
                KeyCombo(SDL_SCANCODE_DELETE, KMOD_CTRL | KMOD_RGUI),
                {
                    KeyCombo(SDL_SCANCODE_DELETE, KMOD_LCTRL | KMOD_RGUI),
                    KeyCombo(SDL_SCANCODE_DELETE, KMOD_RCTRL | KMOD_RGUI),
                }
            }
       ));
}