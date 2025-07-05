#include <minire/formats/bdf.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <cctype>
#include <fstream>

namespace minire::formats
{
    // Bdf::Char //

    void Bdf::Char::reset()
    {
        _name.clear();
        _encoding = 0;
        _swx0 =
        _swy0 =
        _dwx0 =
        _dwy0 =
        _swx1 =
        _swy1 =
        _dwx1 =
        _dwy1 =
        _bbx._w =
        _bbx._h =
        _bbx._xOff =
        _bbx._yOff = 
        _bits = 0;
        _loaded = false;
        _bitLines.clear();
    }

    // Bdf //

    std::string::const_iterator Bdf::parseNonSpace(
        std::string::const_iterator const & begin,
        std::string::const_iterator const & end,
        std::string & token)
    {
        std::string::const_iterator it = begin;

        // skip prologue spaces
        while (it != end && ::isspace(*it)) ++it;

        // skip non-space of token
        while (it != end && !::isspace(*it)) ++it;
        token.assign(begin, it);

        return it;
    }

    void Bdf::parse(std::string::const_iterator const & begin,
                    std::string::const_iterator const & end)
    {
        std::string keyword;

        std::string::const_iterator it =
            parseNonSpace(begin, end, keyword);

        if ("STARTFONT" == keyword)
        {
            if (_fontStarted) MINIRE_THROW("font already started: {}", location());
            it = parseNonSpace(it, end, _version);
            _fontStarted = true;
        }
        else if ("ENDFONT" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            _fontStarted = false;
        }
        else if ("FONT" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            it = parseNonSpace(it, end, _fontName);
        }
        else if ("SIZE" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());

            std::string pointSize, xRes, yRes;

            it = parseNonSpace(it, end, pointSize);
            it = parseNonSpace(it, end, xRes);
            it = parseNonSpace(it, end, yRes);

            _pointSize = std::stoull(pointSize);
            _xRes = std::stoull(xRes);
            _yRes = std::stoull(yRes);
        }
        else if ("FONTBOUNDINGBOX" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());

            std::string w, h, xOff, yOff;

            it = parseNonSpace(it, end, w);
            it = parseNonSpace(it, end, h);
            it = parseNonSpace(it, end, xOff);
            it = parseNonSpace(it, end, yOff);

            _bbx._w = std::stoi(w);
            _bbx._h = std::stoi(h);
            _bbx._xOff = std::stoi(xOff);
            _bbx._yOff = std::stoi(yOff);
        }
        else if ("CHARS" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());

            std::string chars;
            it = parseNonSpace(it, end, chars);
        }
        else if ("STARTCHAR" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (_haveCurrent) MINIRE_THROW("char already started: {}", location());

            _current.reset();
            it = parseNonSpace(it, end, _current._name);

            // setup BBX inherited from font (may be overwritten l8r)
            _current._bbx = _bbx;

            _haveCurrent = true;
        }
        else if ("ENCODING" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (!_haveCurrent) MINIRE_THROW("char already ended: {}", location());

            std::string encoding;
            it = parseNonSpace(it, end, encoding);
            _current._encoding = std::stoull(encoding);

            it = end; // i.e. skipping alternative
        }
        else if ("SWIDTH" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (!_haveCurrent) MINIRE_THROW("char already ended: {}", location());

            std::string swx0, swy0;
            it = parseNonSpace(it, end, swx0);
            it = parseNonSpace(it, end, swy0);

            _current._swx0 = std::stoi(swx0);
            _current._swy0 = std::stoi(swy0);
        }
        else if ("DWIDTH" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (!_haveCurrent) MINIRE_THROW("char already ended: {}", location());

            std::string dwx0, dwy0;
            it = parseNonSpace(it, end, dwx0);
            it = parseNonSpace(it, end, dwy0);

            _current._dwx0 = std::stoi(dwx0);
            _current._dwy0 = std::stoi(dwy0);
        }
        else if ("SWIDTH1" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (!_haveCurrent) MINIRE_THROW("char already ended: {}", location());

            std::string swx1, swy1;
            it = parseNonSpace(it, end, swx1);
            it = parseNonSpace(it, end, swy1);

            _current._swx1 = std::stoi(swx1);
            _current._swy1 = std::stoi(swy1);
        }
        else if ("DWIDTH1" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (!_haveCurrent) MINIRE_THROW("char already ended: {}", location());

            std::string dwx1, dwy1;
            it = parseNonSpace(it, end, dwx1);
            it = parseNonSpace(it, end, dwy1);

            _current._dwx1 = std::stoi(dwx1);
            _current._dwy1 = std::stoi(dwy1);
        }
        else if ("BBX" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (!_haveCurrent) MINIRE_THROW("char already ended: {}", location());

            std::string w, h, xOff, yOff;

            it = parseNonSpace(it, end, w);
            it = parseNonSpace(it, end, h);
            it = parseNonSpace(it, end, xOff);
            it = parseNonSpace(it, end, yOff);

            _current._bbx._w = std::stoi(w);
            _current._bbx._h = std::stoi(h);
            _current._bbx._xOff = std::stoi(xOff);
            _current._bbx._yOff = std::stoi(yOff);
        }
        else if ("BITMAP" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (!_haveCurrent) MINIRE_THROW("char already ended: {}", location());
            if (_haveBitmap) MINIRE_THROW("bitmap already started: {}", location());

            _haveBitmap = true;
        }
        else if ("ENDCHAR" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (!_haveCurrent) MINIRE_THROW("char already ended: {}", location());

            assert(_current._bitLines.size() == _current._bbx._h);

            _current._loaded = true;
            _chars.resize(_current._encoding + 1);
            _chars[_current._encoding] = std::move(_current);
            ++_loadedChars;

            _haveCurrent = false;
            _haveBitmap = false;
        }
        else if ("COMMENT" == keyword)
        {
            // just ignore
            it = end;
        }
        else if ("STARTPROPERTIES" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (_propertiesStarted) MINIRE_THROW("properties already started: {}", location());

            _propertiesStarted = true;
            it = end;
        }
        else if ("ENDPROPERTIES" == keyword)
        {
            if (!_fontStarted) MINIRE_THROW("font already ended: {}", location());
            if (!_propertiesStarted) MINIRE_THROW("properties already ended: {}", location());

            _propertiesStarted = false;
        }
        else if (_fontStarted && !_haveCurrent && !_haveBitmap && _propertiesStarted)
        {
            // skip spaces after keyword
            while (it != end && ::isspace(*it)) ++it;

            std::string value(it, end);
            _properties[keyword] = std::move(value);

            it = end;
        }
        else if (_fontStarted && _haveCurrent && _haveBitmap && !_propertiesStarted)
        {
            size_t idx;
            size_t bitLine = std::stoull(keyword, &idx, 16);
            if (idx != keyword.size())
            {
                MINIRE_THROW("bitmap line expected but got \"{}\": {}",
                             keyword, location());
            }

            _current._bits = keyword.size() * 4;
            _current._bitLines.push_back(bitLine);
        }
        else if (!keyword.empty())
        {
            MINIRE_WARNING("unexpected keyword: \"{}\": {}", keyword, location());
            it = end;
        }

        // skip residual spaces
        while (it != end && ::isspace(*it)) ++it;

        assert(it == end);
    }

    std::string Bdf::location(void) const
    {
        return _filename + ":" + std::to_string(_lineNumber);
    }

    void Bdf::load(std::istream & is)
    {
        std::string line;
        while (std::getline(is, line))
        {
            parse(line.cbegin(), line.cend());
            ++_lineNumber;
        }
    }

    Bdf::Bdf(std::string const & filename)
        : _filename(filename)
    {
        std::ifstream is(_filename);
        if (!is) MINIRE_THROW("cannot open: \"{}\"", _filename);
        load(is);
    }

    Bdf::Bdf(std::istream & is,
             std::string const & filename)
        : _filename(filename)
    {
        load(is);
    }

    void Bdf::fillChar(size_t encoding, size_t value)
    {
        if (encoding < _chars.size())
        {
            Char & glyph = _chars[encoding];
            if (!glyph._loaded)
            {
                MINIRE_THROW("cannot fillChar {}, not loaded",
                             encoding);
            }

            //size_t v = value ? std::numeric_limits<size_t>::max() : 0;
            for(size_t & i : glyph._bitLines)
            {
                i = value;
            }
        }
    }
}
