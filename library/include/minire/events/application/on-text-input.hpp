#pragma once

#include <string>

namespace minire::events::application
{
    struct OnTextInput
    {
        std::string _text; // utf-8

        explicit OnTextInput(std::string text)
            : _text(std::move(text))
        {}
    };
}
