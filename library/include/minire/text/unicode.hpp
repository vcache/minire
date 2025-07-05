#pragma once

#include <string>

namespace minire::text
{
    std::string toUtf8(std::wstring const & in);
    std::wstring toUnicode(std::string const & in);
}
