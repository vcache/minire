#pragma once

#include <iostream>

#include <fmt/format.h>

namespace minire::logging
{
    enum class Level
    {
        kDebug   = 0,
        kInfo    = 1,
        kWarning = 2,
        kError   = 3,
    };

    void setVerbosity(Level minLevelToPrint);

    Level getVerbosity();

    std::string prefix(Level level,
                       char const * file,
                       int line,
                       char const * function);

    void streamLines(std::ostream & os,
                     std::string const & text,
                     size_t pfxLen);

    template<typename... FmtArgs>
    void log(Level level,
             char const * file,
             int line,
             char const * function,
             fmt::format_string<FmtArgs...> fmtFormat,
             FmtArgs && ... fmtArgs)
    {
        if (static_cast<int>(getVerbosity()) <= static_cast<int>(level))
        {
            std::string pfx = prefix(level, file, line, function);
            std::string text = fmt::format(fmtFormat, std::forward<FmtArgs>(fmtArgs)...);

            std::cout << pfx;
            streamLines(std::cout, text, pfx.size());
            std::cout.flush();
        }
    }

    // TODO: set custom callback
}

#define MINIRE_LOG(level, format, ...)                                                              \
    if (static_cast<int>(::minire::logging::getVerbosity()) <= static_cast<int>(level)) {           \
        ::minire::logging::log(level,  __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__);    \
    }

#ifndef NDEBUG
#   define MINIRE_DEBUG(format, ...) MINIRE_LOG(::minire::logging::Level::kDebug, format, ##__VA_ARGS__)
#else
#   define MINIRE_DEBUG(format, ...) {}
#endif

#define MINIRE_INFO(format, ...) MINIRE_LOG(::minire::logging::Level::kInfo, format, ##__VA_ARGS__)
#define MINIRE_WARNING(format, ...) MINIRE_LOG(::minire::logging::Level::kWarning, format, ##__VA_ARGS__)
#define MINIRE_ERROR(format, ...) MINIRE_LOG(::minire::logging::Level::kError, format, ##__VA_ARGS__)


