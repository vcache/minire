#pragma once

#include <minire/text/text-format.hpp>

namespace minire::text
{
    // TODO: tests
    class Symbol : public TextFormat
    {
    public:
        Symbol(TextFormat const & f, wchar_t c)
            : TextFormat(f)
            , _codePoint(c)
        {}

        Symbol()
            : _codePoint(L'\0')
        {}

    public:
        void unset() { blank(true); }
        
        void set(wchar_t c) { _codePoint = c; blank(false); }            
        
        void set(TextFormat const & f, wchar_t c)
        {
            static_cast<TextFormat&>(*this) = f;
            _codePoint = c;
            blank(false); // TODO: wtf?
        }
        
    public:
        wchar_t codePoint() const { return _codePoint; }

        bool operator==(Symbol const & other) const
        {
            auto const & thisFormat = static_cast<TextFormat const &>(*this);
            auto const & otherFormat = static_cast<TextFormat const &>(other);
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
