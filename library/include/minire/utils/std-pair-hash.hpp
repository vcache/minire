#pragma once

#include <functional> // For std::hash
#include <utility>
    
#include <boost/container_hash/hash.hpp> // for hash_combine

namespace std
{
    template<typename K, typename V>
    struct hash<pair<K, V>>
    {
        size_t operator()(pair<K, V> const & v) const
        {
            size_t result = 0x5D239016BE5C2176ULL;
            boost::hash_combine(result, std::hash<K>{}(v.first));
            boost::hash_combine(result, std::hash<V>{}(v.second));
            return result;
        }
    };
}
