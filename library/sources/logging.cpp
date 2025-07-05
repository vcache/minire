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
                case Level::kDebug:   return "DEBUG";
                case Level::kInfo:    return "INFO ";
                case Level::kWarning: return "WARN ";
                case Level::kError:   return "ERROR";
            }
            return "UNKNOWN";
        }
    }

    // TODO: check if logs is printed to tty, and if so, break lines on a border of terminal
    void streamLines(std::ostream & os, std::string const & text, size_t prefixLen)
    {
        size_t pos = 0;
        std::string::size_type eol;

        do
        {
            if (pos != 0)
            {
                for(size_t i = 0; i < prefixLen; ++i) os.put(' ');
            }

            eol = text.find('\n', pos);
            if (std::string::npos == eol) eol = text.size();
            os.write(text.c_str() + pos, (eol - pos));
            os << '\n';
            pos = eol + 1;
        }
        while (pos < text.size());
    }

    // TODO: check if colors are supported, and if so, colorize them
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


