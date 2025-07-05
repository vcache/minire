#pragma once

#include <cassert>
#include <cstdint>
#include <istream>
#include <map>
#include <memory>
#include <string>
#include <vector>

// TODO: cover with tests

namespace minire::formats
{
    class Bdf
    {
        using Properties = std::map<std::string, std::string>;

        struct BBox
        {
            size_t _w    = 0;
            size_t _h    = 0;
            int    _xOff = 0;
            int    _yOff = 0;
        };

        class Char // TODO: rename to Glyph
        {
            friend class Bdf;

            using BitLines = std::vector<size_t>;

            std::string _name;
            size_t      _encoding = 0;
            int         _swx0 = 0;
            int         _swy0 = 0;
            int         _dwx0 = 0;
            int         _dwy0 = 0;
            int         _swx1 = 0;
            int         _swy1 = 0;
            int         _dwx1 = 0;
            int         _dwy1 = 0;
            BBox        _bbx;
            size_t      _bits = 0;
            bool        _loaded = false;
            BitLines    _bitLines;

            void reset();

        public:
            bool         loaded() const { return _loaded; }
            BBox const & bbox() const   { return _bbx; }

            size_t const & bitLine(size_t y) const
            {
                assert(y < _bitLines.size());
                return _bitLines[y];
            }

            bool test(size_t x, size_t y) const
            {
                assert(_bitLines.size() == _bbx._h);
                return x < _bbx._w &&
                       y < _bitLines.size() && (
                        _bitLines[y] & (size_t(1) << (_bits - x - 1)));
            }

            template<typename Pixel, Pixel kOn, Pixel kOff>
            void render(Pixel * buffer,
                               size_t bufWidth,
                               size_t /*bufHeight*/,
                               size_t stX,
                               size_t stY) const
            {
                for(size_t y = 0; y < _bbx._h; ++y)
                {
                    for(size_t x = 0; x < _bbx._w; ++x)
                    {
                        buffer[(stX + x) + (stY + y) * bufWidth] =
                            test(x, y) ? kOn : kOff;
                    }
                }
            }
        };

    public:
        using Sptr = std::shared_ptr<Bdf>;

        explicit Bdf(std::string const &);

        explicit Bdf(std::istream &,
                     std::string const &);

    public:
        BBox const & bbox() const { return _bbx; }

    public:
        Char const & find(size_t encoding) const
        {
            static Char kNotFound;
            return encoding < _chars.size() ? _chars[encoding]
                                            : kNotFound;
        }

        void fillChar(size_t encoding, size_t value);

        size_t maxEncoding() const { return _chars.size() - 1; }
        size_t loadedChars() const { return _loadedChars; }

        auto const & filename() const { return _filename; }

    private:
        static std::string::const_iterator parseNonSpace(
            std::string::const_iterator const & begin,
            std::string::const_iterator const & end,
            std::string & token);

        void load(std::istream &);

        void parse(std::string::const_iterator const & begin,
                   std::string::const_iterator const & end);

        std::string location(void) const;

    private:
        using Chars = std::vector<Char>;

        bool        _fontStarted = false;
        bool        _propertiesStarted = false;
        std::string _version;
        std::string _fontName;
        size_t      _loadedChars = 0;
        size_t      _pointSize = 0;
        size_t      _xRes = 0;
        size_t      _yRes = 0;
        BBox        _bbx;
        Chars       _chars;
        Properties  _properties;

        Char        _current;
        bool        _haveCurrent = false;
        bool        _haveBitmap = false;

        size_t      _lineNumber = 0;
        std::string _filename;
    };
}
