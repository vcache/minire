#include <minire/system/os.hpp>

#include <minire/errors.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <lmcons.h>

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
}