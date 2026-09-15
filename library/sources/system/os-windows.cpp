#include <minire/system/os.hpp>

#include <minire/errors.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <lmcons.h>
#include <shlobj.h>
#include <knownfolders.h>

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
}
