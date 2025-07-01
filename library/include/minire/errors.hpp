/**
 * \file exception.hpp
 * \author Igor Bereznyak <igor.bereznyak@gmail.com>
 */
#pragma once

#include <stdexcept>
#include <string>
#include <utility> // for std::forward

#include <fmt/format.h>

namespace minire
{
    template<typename... Args>
    std::string formatNoExc(fmt::format_string<Args...> format,
                            Args && ... args) noexcept
    {
        try
        {
            return fmt::format(format, std::forward<Args>(args)...);
        }
        catch(...)
        {
            return "(failed to format a message)"; // TODO: this may throw anyway
        }
    }

    char const * cutFilePrefix(char const * full);

    // TODO: see https://en.cppreference.com/w/cpp/utility/source_location
    class Exception : public std::exception
    {
    public:
        Exception(int line,
                  char const * file,
                  char const * call,
                  std::string && what) noexcept
            : _line(line)
            , _file(file)
            , _call(call)
            , _what(std::move(what))
        {}

        int line() const noexcept
        {
            return _line;
        }

        char const * file() const noexcept
        {
            return _file;
        }

        char const * call() const noexcept
        {
            return _call;
        }

        char const * what() const noexcept override
        {
            return evalFull() ? _full.c_str()
                              : _what.c_str();
        }

        char const * realWhat() const noexcept
        {
            return _what.c_str();
        }

    private:
        bool evalFull() const noexcept
        {
            if (!_full.empty()) return true;

            try
            {
                _full = fmt::format("[{}:{}]: {}", cutFilePrefix(_file), _line, _what);
                return true;
            }
            catch(...)
            {
                return false;
            }
        }

    private:
        int const           _line;
        char const        * _file;
        char const        * _call;
        std::string         _what;
        mutable std::string _full;
    };

    class FailedInvariant : public Exception
    {
        using Exception::Exception;
    };

    class RuntimeError : public Exception
    {
        using Exception::Exception;
    };
}

#define MINIRE_INVARIANT(invariant, message, ...)                                   \
    if (invariant) {                                                                \
    } else {                                                                        \
        throw ::minire::FailedInvariant(__LINE__, __FILE__, __PRETTY_FUNCTION__,    \
            ::minire::formatNoExc("An invariant failed"                             \
                                   " [" # invariant "]: " message,                  \
                                   ##__VA_ARGS__));                                 \
    }

#define MINIRE_THROW(message, ...)                                                  \
    throw ::minire::RuntimeError(__LINE__, __FILE__, __PRETTY_FUNCTION__,           \
        ::minire::formatNoExc("Runtime error: " message,                            \
                               ##__VA_ARGS__));
