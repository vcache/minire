#include <minire/system/os.hpp>

#include <minire/errors.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <lmcons.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <winnls.h>

#include <algorithm>
#include <cctype>

namespace minire::system
{
    std::string getUsername()
    {
        char buffer[UNLEN + 1];
        DWORD size = sizeof(buffer);

        if (!::GetUserNameA(buffer, &size))
        {
            auto err = ::GetLastError();
            MINIRE_THROW("GetUserNameA failed: error code {}", err);
        }

        return buffer;
    }

    int getTid()
    {
        return static_cast<int>(::GetCurrentThreadId());
    }

    std::filesystem::path getUserDirectory()
    {
        PWSTR widePath = nullptr;

        // FOLDERID_LocalAppData points to C:\Users\<User>\AppData\Local
        // KF_FLAG_CREATE ensures the directory exists on disk
        HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_LocalAppData,
                                            KF_FLAG_CREATE,
                                            nullptr, &widePath);
        if (FAILED(hr))
        {
            MINIRE_THROW("SHGetKnownFolderPath failed with HRESULT: 0x{:08X}",
                         static_cast<unsigned long>(hr));
        }

        std::filesystem::path result(widePath);
        ::CoTaskMemFree(widePath);

        return result;
    }

    std::string getUserLanguage()
    {
        ULONG numLanguages = 0;
        ULONG bufferSize = 0;

        // query the preferred UI languages (e.g. L"en-US\0ja-JP\0\0")
        if (::GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, nullptr, &bufferSize);
            bufferSize > 0)
        {
            std::wstring buffer(bufferSize, L'\0');
            if (::GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numLanguages, buffer.data(), &bufferSize);
                numLanguages > 0)
            {
                std::string lang;
                for (wchar_t wc : buffer)
                {
                    if (wc == L'-' || wc == L'_' || wc == L'\0')
                    {
                        break;
                    }
                    lang.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(wc))));
                }

                if (!lang.empty())
                {
                    return lang;
                }
            }
        }

        // fallback: query default locale name (e.g. L"en-US")
        wchar_t localeName[LOCALE_NAME_MAX_LENGTH] = {0};
        if (::GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH) > 0)
        {
            std::string lang;
            for (wchar_t wc : localeName)
            {
                if (wc == L'-' || wc == L'_' || wc == L'\0')
                {
                    break;
                }
                lang.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(wc))));
            }

            if (!lang.empty())
            {
                return lang;
            }
        }

        return "en";
    }
}
