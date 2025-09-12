#include <minire/instrumentation/formatters/table.hpp>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <string>
#include <vector>

namespace minire::instrumentation::formatters
{
    std::string const & Table::Row::at(size_t i) const
    {
        static const std::string kNone;
        return i < _cols.size() ? _cols[i] : kNone;
    }

    Table::Table(std::string title,
                 std::vector<std::string> header)
        : _title(title)
        , _header(header)
    {}

    std::vector<std::string> Table::format() const
    {
        std::vector<size_t> lengths;
        lengths.reserve(_header.size());

        size_t const columns = _header.size();

        for(size_t col = 0; col < columns; ++col)
        {
            size_t colLen = _header[col].size();
            for(Row const & row : _rows)
            {
                assert(row.columns() == columns);
                colLen = std::max(colLen, row.at(col).size());
            }
            lengths.push_back(colLen);
        }

        std::vector<std::string> result;
        result.reserve(3 + 1 + _rows.size());

        std::string sepLine;
        {
            for(size_t i = 0; i < columns; ++i)
            {
                sepLine += std::string("+") + std::string(2 + lengths[i], '-');
            }
            sepLine += "+";
        }

        size_t const fullLength = (
            std::accumulate(lengths.cbegin(), lengths.cend(), 0) + 1 + 3*lengths.size());

        result.push_back(std::string("+") + std::string(fullLength - 2, '-') + "+");

        result.push_back(padded("| ", _title, sepLine.size() - 3) + "|");

        result.push_back(sepLine);

        {
            std::string line;
            for(size_t i = 0; i < columns; ++i)
            {
                line += padded("| ", _header[i], lengths[i] + 1);
            }
            result.push_back(line + "|");
        }

        result.push_back(sepLine);

        for(Row const & row : _rows)
        {
            std::string line;
            for(size_t i = 0; i < columns; ++i)
            {
                line += padded("| ", row.at(i), lengths[i] + 1);
            }
            result.push_back(line + "|");
        }

        result.push_back(sepLine);

        return result;
    }

    size_t Table::lines() const
    {
        return 6 + _rows.size();
    }

    std::string Table::padded(std::string const & prefix,
                              std::string const & data,
                              size_t length) const
    {
        std::string result = prefix + data;
        if (data.size() < length)
        {
            result += std::string(length - data.size(), ' ');
        }
        return result;
    }
}
