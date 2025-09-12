#pragma once

#include <minire/instrumentation/formatters/table.hpp>
#include <minire/instrumentation/histogram.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <cassert>
#include <string>

namespace minire::instrumentation
{
    template<typename T>
    std::string tabulate(typename Histogram<T>::Aggregation const & aggregation)
    {
        std::vector<std::string> header;
        header.push_back("Stopwatch");
        header.push_back("Events, pcs"); // pcs are "pieces", not "parsecs"
        for(double perc : aggregation._percentiles)
        {
            header.push_back(fmt::format("{:.2f}%, s", perc * 100.0));
        }

        std::string title = fmt::format("Histogram aggregation, window is {:.3f}s",
                                        aggregation._timeWindow.count());
        formatters::Table table(title, header);
        for(auto const & [name, statistics] : aggregation._statistics)
        {
            auto & row = table.insert()("{}", name)
                                       ("{}", statistics._measurements);
            assert(statistics._percentiles.size() == aggregation._percentiles.size());
            for(double const perc : statistics._percentiles)
            {
                row("{:.6f}", perc);
            }
        }

        return fmt::format("{}", fmt::join(table.format(), "\n"));
    }
}