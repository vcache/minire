#pragma once

#include <boost/container_hash/hash.hpp> // for hash_combine

#include <functional> // For std::hash
#include <string>
#include <vector>

namespace minire::models
{
    using ScenePath = std::vector<std::string>;

    ScenePath concat(ScenePath const & a, ScenePath const & b);
    ScenePath concat(ScenePath const & a, std::string const & b);
    ScenePath cutPrefix(ScenePath const & full, ScenePath const & prefix);

    template<typename... Args>
    ScenePath mkScenePath(Args && ... args)
    {
        return ScenePath{ {ScenePath::value_type(std::forward<Args>(args))...} };
    }
}

namespace std
{
    template<>
    struct hash<::minire::models::ScenePath>
    {
        size_t operator()(::minire::models::ScenePath const & v) const
        {
            size_t result = 0xCB39ACEFA8402761ULL;
            for(std::string const & i : v)
            {
                // TODO: add index into a hash
                boost::hash_combine(result, std::hash<std::string>{}(i));
            }
            return result;
        }
    };
}
