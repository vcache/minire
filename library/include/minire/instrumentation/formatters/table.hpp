#pragma once

#include <fmt/format.h>

#include <string>
#include <vector>

namespace minire::instrumentation::formatters
{
    class Table
    {
    public:
        class Row
        {
        public:
            template<typename... Args>
            Row & operator()(fmt::format_string<Args...> formatString,
                             Args && ... args)
            {
                _cols.push_back(
                    fmt::format(formatString, std::forward<Args>(args)...));
                return *this;
            }

            std::string const & at(size_t i) const;
            size_t columns() const { return _cols.size(); }

        private:
            std::vector<std::string> _cols;
        };

    public:
        explicit Table(std::string title,
                       std::vector<std::string> header);

        Row & insert()
        {
            _rows.push_back(Row());
            return _rows.back();
        }

        std::vector<std::string> format() const;

        size_t lines() const;

    private:
        std::string padded(std::string const & prefix,
                           std::string const & data,
                           size_t length) const;

    private:
        std::string const        _title;
        std::vector<std::string> _header;
        std::vector<Row>         _rows;
    };
}
