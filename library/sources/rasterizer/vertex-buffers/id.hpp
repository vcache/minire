#pragma once

#include <minire/content/id.hpp>

#include <boost/container_hash/hash.hpp> // for hash_combine

#include <functional> // For std::hash

namespace minire::rasterizer::vertex_buffers
{
    struct Id
    {
        content::Id _contentId;

        bool operator==(Id const & o) const
        {
            return std::tie(  _contentId)
                == std::tie(o._contentId);
        }
    };
}

namespace std
{
    template<>
    struct hash<::minire::rasterizer::vertex_buffers::Id>
    {
        size_t operator()(::minire::rasterizer::vertex_buffers::Id const & v) const
        {
            return std::hash<::minire::content::Id>{}(v._contentId);
        }
    };
}
