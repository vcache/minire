#pragma once

#include <string>

namespace minire::events::controller
{
    struct StartClipboardCapture
    {};

    struct StopClipboardCapture
    {};

    struct SetClipboardText
    {
        std::string _text;
    };

    struct SetPrimarySelection
    {
        std::string _text;
    };
}
