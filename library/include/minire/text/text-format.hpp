#pragma once

#include <glm/vec4.hpp>

#include <cstdint>

namespace minire::text
{
    // TODO: tests
    class Format
    {
    protected:
        static constexpr uint8_t kBlank        = (1 << 0);
        static constexpr uint8_t kBold         = (1 << 1);
        static constexpr uint8_t kItalic       = (1 << 2);
        static constexpr uint8_t kUnderline    = (1 << 3);
        static constexpr uint8_t kStrikeout    = (1 << 4);
        static constexpr uint8_t kInvertColors = (1 << 5);

    public:
        bool operator==(Format const & other) const
        {
            return _foreground == other._foreground
                && _background == other._background
                && _flags == other._flags;
        }

    public:
        bool blank() const { return _flags & kBlank; }
        bool bold() const { return _flags & kBold; }
        bool italic() const { return _flags & kItalic; }
        bool underline() const { return _flags & kUnderline; }
        bool strikeout() const { return _flags & kStrikeout; }
        bool invertColors() const { return _flags & kInvertColors; }

        glm::vec4 const & foreground() const { return _foreground; }
        glm::vec4 const & background() const { return _background; }

    public:
        Format & blank(bool v)        { return _s(v, kBlank); }
        Format & bold(bool v)         { return _s(v, kBold); }
        Format & italic(bool v)       { return _s(v, kItalic); }
        Format & underline(bool v)    { return _s(v, kUnderline); }
        Format & strikeout(bool v)    { return _s(v, kStrikeout); }
        Format & invertColors(bool v) { return _s(v, kInvertColors); }

        Format & foreground(glm::vec4 const & v)
        {
            _foreground = v;
            return *this;
        }

        Format & background(glm::vec4 const & v)
        {
            _background = v;
            return *this;
        }

    private:
        Format & _s(bool val, uint8_t flag)
        {
            if (val)
            {
                _flags |= flag;
            }
            else
            {
                _flags &= ~flag;
            }
            return *this;
        }

    private:
        glm::vec4 _foreground = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec4 _background = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        uint8_t   _flags      = 0; // TODO: why not std::bitset?
    };
}
