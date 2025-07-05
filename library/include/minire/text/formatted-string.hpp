#pragma once

#include <minire/text/text-format.hpp>
#include <minire/text/unicode.hpp>

#include <string>
#include <vector>
#include <utility>
#include <cassert>

namespace minire::text
{
    // TODO: tests
    class FormattedString
    {
        friend class Iterator;

        using Fragment = std::pair<TextFormat, std::wstring>;
        using Fragments = std::vector<Fragment>;

    public:
        FormattedString(FormattedString const &) = default;

        FormattedString(FormattedString && other)
            : _fragments(std::move(other._fragments))
            , _size(other._size)
        {
            other._size = 0;
        }

        FormattedString(wchar_t const * s)
        {
            append(std::wstring(s));
        }

        FormattedString(std::wstring const & s)
        {
            append(s);
        }

        FormattedString(std::wstring && s)
        {
            append(std::move(s));
        }

    public:
        FormattedString & operator=(FormattedString && other)
        {
            FormattedString tmp(std::move(other));
            std::swap(_fragments, tmp._fragments);
            std::swap(_size, tmp._size);
            return *this;
        }

    public:
        TextFormat & append(std::wstring && s)
        {
            _size += s.size();
            _fragments.emplace_back(TextFormat(false),
                                    std::move(s));
            return _fragments.back().first;
        }

        TextFormat & append(std::wstring const & s)
        {
            _size += s.size();
            _fragments.emplace_back(TextFormat(false), s);
            return _fragments.back().first;
        }

        TextFormat & append(std::string const & s)
        {
            return append(toUnicode(s));
        }

        size_t size() const { return _size; }

        std::wstring wunformat() const
        {
            std::wstring result;
            for(Fragment const & fragment : _fragments)
            {
                result += fragment.second;
            }
            return result;
        }

        std::string unformat() const
        {
            return toUtf8(wunformat());
        }

    public:
        class Iterator
        {
            friend class FormattedString;

            size_t                  _fragment;
            size_t                  _offset;
            FormattedString const & _formattedString;
        
            inline
            Iterator(size_t fragment,
                     size_t offset,
                     FormattedString const & formattedString)
                : _fragment(fragment)
                , _offset(offset)
                , _formattedString(formattedString)
            {}

        public:
            std::pair<TextFormat const &, wchar_t> operator*() const
            {
                assert(_fragment < _formattedString._fragments.size());

                Fragment const & fragment =
                    _formattedString._fragments[_fragment];

                assert(_offset < fragment.second.size());
                return std::pair<TextFormat const &, wchar_t>(
                    fragment.first,
                    fragment.second[_offset]);
            }

            Iterator& operator++()
            {
                assert(_fragment < _formattedString._fragments.size());

                Fragment const & fragment =
                    _formattedString._fragments[_fragment];

                ++_offset;
                if (_offset >= fragment.second.size())
                {
                    _offset = 0;
                    ++_fragment;
                }

                return *this;
            }

            bool operator==(Iterator const & other) const
            {
                return _fragment == other._fragment
                    && _offset == other._offset;
            }

            bool operator!=(Iterator const & other) const
            {
                return !operator==(other);
            }
        };

        Iterator begin() const
        {
            return Iterator(0, 0, *this);
        }
        
        Iterator end() const
        {
            return Iterator(_fragments.size(), 0, *this);
        }

    private:
        Fragments _fragments;
        size_t    _size = 0;
    };
}
