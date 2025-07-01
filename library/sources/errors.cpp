#include <minire/errors.hpp>

#include <cstring>

namespace minire
{
    char const * cutFilePrefix(char const * full)
    {
        if (nullptr != std::strstr(full, CMAKE_CURRENT_SOURCE_DIR))
        {
            return full + sizeof(CMAKE_CURRENT_SOURCE_DIR);
        }
        else
        {
            return full;
        }
    }
}
