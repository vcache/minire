#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minire::instrumentation
{
    template<typename T = double>
    class Histogram
    {
    public:
        using Sptr = std::shared_ptr<Histogram>;

        explicit Histogram(std::chrono::duration<double> timeWindow,
                           std::vector<double> percentiles = {0.00, 0.50, 0.98, 0.99, 0.9999, 1.00})
            : _timeWindow(timeWindow)
            , _percentiles(percentiles)
            , _begin(std::chrono::steady_clock::now())
        {}

        void collectMeasurement(std::string const & name, T const time)
        {
            auto & store = _values[name];
            store.reserve(_maxSerie);
            store.push_back(time);
        }

        bool isExpired() const
        {
            auto const now = std::chrono::steady_clock::now();
            return now - _begin >= _timeWindow;
        }

        auto begin() const { return _values.begin(); }

        auto end() const { return _values.end(); }

        bool empty() const { return _values.empty(); }

        auto const & percentiles() const { return _percentiles; }

    public:
        struct Statistics
        {
            std::vector<T> _percentiles;
            size_t         _measurements;
        };

        struct Aggregation
        {
            std::vector<double>                         _percentiles;
            std::unordered_map<std::string, Statistics> _statistics;
            std::chrono::duration<double>               _timeWindow;
        };

        std::unique_ptr<Aggregation> fetch()
        {
            if (!isExpired()) return {};

            for(auto & [_, values] : _values)
            {
                std::ranges::sort(values);
            }

            std::unique_ptr<Aggregation> result = std::make_unique<Aggregation>();
            result->_statistics.reserve(_values.size());
            result->_percentiles = percentiles();
            result->_timeWindow = _timeWindow;
            for(auto const & [name, values] : _values)
            {
                auto & statistics = result->_statistics[name];
                statistics._percentiles.reserve(_percentiles.size());
                for(double const percentile : _percentiles)
                {
                    // TODO: a true percentile should average value between indeces
                    size_t const index = percentile * static_cast<double>(values.size() - 1);
                    assert(index < values.size());
                    statistics._percentiles.push_back(values[index]);
                }
                statistics._measurements = values.size();

                _maxSerie = std::max(_maxSerie, values.size());
            }

            _values.clear();
            _begin = std::chrono::steady_clock::now();

            return result;
        }

    private:
        std::chrono::duration<double> const                _timeWindow;
        std::vector<double> const                          _percentiles;
        std::chrono::time_point<std::chrono::steady_clock> _begin;
        std::unordered_map<std::string, std::vector<T>>    _values;
        size_t                                             _maxSerie = 0;
    };
}