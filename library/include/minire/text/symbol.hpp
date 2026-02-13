#pragma once

#include <minire/text/text-format.hpp>

namespace minire::text
{
    // TODO: tests
    // TODO: maybe do composition instead inheritance,
    //       thereby format can be stored as shared_ptr<Format>,
    //       so, symbols can share same Format instead copies.
    class Symbol : public Format
    {
    public:
        Symbol(wchar_t c, Format const & fmt)
            : Format(fmt)
            , _codePoint(c)
        {}

        Symbol()
            : _codePoint(L'\0')
        {
            blank(true);
        }

    public:
        void unset() { blank(true); }

        void set(wchar_t c) { _codePoint = c; blank(false); }

        void set(Format const & f, wchar_t c)
        {
            static_cast<Format&>(*this) = f;
            _codePoint = c;
            blank(false); // TODO: wtf?
        }

    public:
        wchar_t codePoint() const { return _codePoint; }

        bool operator==(Symbol const & other) const
        {
            auto const & thisFormat = static_cast<Format const &>(*this);
            auto const & otherFormat = static_cast<Format const &>(other);
            return _codePoint == other._codePoint
                && thisFormat == otherFormat;
        }

        bool operator!=(Symbol const & other) const
        {
            return !operator==(other);
        }

    private:
        size_t _codePoint;
    };
}
