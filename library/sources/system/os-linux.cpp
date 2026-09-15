#include <minire/system/os.hpp>

#include <minire/errors.hpp>

#include <cstdlib>
#include <cstring>

#include <pwd.h>
#include <sys/syscall.h>
#include <unistd.h>
#define gettid() syscall(SYS_gettid)

namespace minire::system
{
    std::string getUsername()
    {
        char buf[128];
        if (0 != ::getlogin_r(buf, sizeof(buf)))
        {
            auto e = errno;
            MINIRE_THROW("getlogin_r failed: {}", ::strerror(e));
        }
        return buf;
    }

    int getTid()
    {
        return gettid();
    }

    std::filesystem::path getUserDirectory()
    {
        // Check $XDG_CONFIG_HOME
        const char * xdgConfig = std::getenv("XDG_CONFIG_HOME");
        if (xdgConfig && *xdgConfig != '\0')
        {
            return std::filesystem::path(xdgConfig);
        }

        // Fallback to $HOME/.config
        const char * home = std::getenv("HOME");
        if (home && *home != '\0')
        {
            return std::filesystem::path(home) / ".config";
        }

        // Fallback to /etc/passwd entry
        struct passwd * pw = ::getpwuid(::getuid());
        if (pw && pw->pw_dir)
        {
            return std::filesystem::path(pw->pw_dir) / ".config";
        }

        MINIRE_THROW("Could not resolve user home directory");
    }
}
