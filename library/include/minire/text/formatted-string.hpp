#pragma once

#include <minire/text/symbol.hpp>
#include <minire/text/text-format.hpp>
#include <minire/text/unicode.hpp>

#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace minire::text
{
    // TODO: tests
    class FormattedString
    {
    public:
        FormattedString() = default;

        FormattedString(char const * s,
                        Format const & fmt = {})
        {
            assign(std::string(s), fmt);
        }

        FormattedString(wchar_t const * s,
                        Format const & fmt = {})
        {
            assign(std::wstring(s), fmt);
        }

        FormattedString(std::string const & s,
                        Format const & fmt = {})
        {
            assign(s, fmt);
        }

        FormattedString(std::wstring const & s,
                        Format const & fmt = {})
        {
            assign(s, fmt);
        }

    public:
        void assign(std::wstring const & s,
                    Format const & fmt)
        {
            _store.clear();
            _store.reserve(s.size());
            for(wchar_t c : s)
            {
                _store.emplace_back(c, fmt);
            }
        }

        void assign(std::string const & s,
                    Format const & fmt)
        {
            assign(toUnicode(s), fmt);
        }

    public:
        FormattedString & operator+=(FormattedString const & other)
        {
            _store.insert(_store.end(), other._store.begin(),
                                        other._store.end());
            return *this;
        }

        void append(wchar_t const s,
                    Format fmt = {})
        {
            _store.emplace_back(s, std::move(fmt));
        }

        void append(std::wstring const & s,
                    Format const & fmt = {})
        {
            this->operator+=(FormattedString(s, fmt));
        }

        void append(std::string const & s,
                    Format const & fmt = {})
        {
            this->operator+=(FormattedString(s, fmt));
        }

    public:
        void insert(size_t pos, wchar_t symbol, Format const & fmt)
        {
            assert(pos <= _store.size());
            _store.emplace(_store.begin() + pos, symbol, fmt);
        }

        void erase(size_t pos)
        {
            assert(pos < _store.size());
            _store.erase(_store.begin() + pos);
        }

        void erase(size_t begin, size_t end /* past-the-end */)
        {
            assert(begin < _store.size());
            assert(end <= _store.size());
            assert(begin <= end);
            _store.erase(_store.begin() + begin,
                         _store.begin() + end);
        }

        void clear() { _store.clear(); }

    public:
        Symbol const & operator[](size_t index) const
        {
            assert(index < _store.size());
            return _store[index];
        }

        Symbol & operator[](size_t index)
        {
            assert(index < _store.size());
            return _store[index];
        }

    public:
        std::wstring wunformat(size_t begin = 0,
                               size_t end = std::numeric_limits<size_t>::max()) const
        {
            end = std::min(end, _store.size());
            assert(begin <= end);

            std::wstring result;
            result.reserve(end - begin);
            for(size_t i = begin; i < end; ++i)
            {
                Symbol const & symbol = _store[i];
                result += symbol.codePoint();
            }
            return result;
        }

        std::string unformat(size_t begin = 0,
                             size_t end = std::numeric_limits<size_t>::max()) const
        {
            return toUtf8(wunformat(begin, end));
        }

    public:
        bool operator==(FormattedString const &) const = default;

    public:
        size_t size() const { return _store.size(); }
        bool empty() const { return _store.empty(); }

    public:
        auto begin() { return _store.begin(); }
        auto end() { return _store.end(); }

        auto begin() const { return _store.begin(); }
        auto end() const { return _store.end(); }

        auto cbegin() const { return _store.cbegin(); }
        auto cend() const { return _store.cend(); }

    private:
        // TODO: may be replate to std::list, it will bring better complexity for insert and delete,
        //       besides, random-order access (operator[]) isn't really required here.
        std::vector<Symbol> _store;
    };
}
