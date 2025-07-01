#include <minire/system/os.hpp>

#include <minire/errors.hpp>

#include <cstring>

#include <unistd.h>
#include <sys/syscall.h>
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
}
