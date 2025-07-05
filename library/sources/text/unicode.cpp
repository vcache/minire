#include <minire/text/unicode.hpp>

#include <boost/locale/encoding_utf.hpp> // TODO: use codecvt instead

namespace minire::text
{
    std::string toUtf8(std::wstring const & in)
    {
        /*
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;
        return converterX.to_bytes(in);
        */
        return boost::locale::conv::utf_to_utf<char>(
            in.c_str(), in.c_str() + in.size());
    }

    std::wstring toUnicode(std::string const & in)
    {
        /*
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;
        return converterX.from_bytes(str);
        */
        return boost::locale::conv::utf_to_utf<wchar_t>(
            in.c_str(), in.c_str() + in.size());
    }
}
