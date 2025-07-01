#include <minire/logging.hpp>

#include <minire/errors.hpp> // for cutFilePrefix
#include <minire/system/os.hpp>

#include <fmt/chrono.h>

#include <chrono>

namespace minire::logging
{
    static Level kMinLevelToPrint = Level::kInfo;

    namespace
    {
        char const * toString(Level level)
        {
            switch(level)
            {
                case Level::kDebug: return "DEBUG";
                case Level::kInfo: return "INFO";
                case Level::kWarning: return "WARNING";
                case Level::kError: return "ERROR";
            }
            return "UNKNOWN";
        }
    }

    void setVerbosity(Level minLevelToPrint)
    {
#       ifdef NDEBUG
        if (kMinLevelToPrint == Level::kDebug)
            return;
#       endif

        kMinLevelToPrint = minLevelToPrint;
    }

    Level getVerbosity()
    {
        return kMinLevelToPrint;
    }

    std::string prefix(Level level,
                       char const * file,
                       int line,
                       char const * /* function */)
    {
        using namespace std::chrono;

        auto now = system_clock::now();
        return fmt::format("{:%Y-%m-%dT%H:%M}:{:%S} {} {} {}:{} - ",
                           now, duration_cast<milliseconds>(now.time_since_epoch()),
                           system::getTid(),
                           toString(level),
                           cutFilePrefix(file),
                           line);
    }
}


