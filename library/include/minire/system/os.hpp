#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace minire::system
{
    std::string getUsername();

    int getTid();

    // - Linux: $XDG_CONFIG_HOME (default: ~/.config)
    // - Windows: %LOCALAPPDATA% (e.g. C:\Users\<Name>\AppData\Local)
    std::filesystem::path getUserDirectory();

    // ISO 639-1 two-letter code, falls back to "en"
    std::string getUserLanguage();
}
