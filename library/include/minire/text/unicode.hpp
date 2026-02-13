#pragma once

#include <string>

namespace minire::text
{
    std::string toUtf8(std::wstring const & in);
    std::wstring toUnicode(std::string const & in);

    // NOTE: don't use L'\u240A' as a line breaker,
    //       because it considered as printable!
    inline bool isLineBreak(wchar_t codePoint)
    {
        return L'\n' == codePoint;
    }
}
