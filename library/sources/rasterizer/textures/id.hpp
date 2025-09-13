#pragma once

#include <minire/content/id.hpp>
#include <minire/models/sampler.hpp>

#include <functional> // For std::hash

#include <boost/container_hash/hash.hpp> // for hash_combine

namespace minire::rasterizer::textures
{
    struct Id
    {
        content::Id     _contentId;
        models::Sampler _sampler;
        bool            _hasMipMaps;

        bool operator==(Id const & o) const
        {
            return std::tie(  _contentId,   _sampler,   _hasMipMaps)
                == std::tie(o._contentId, o._sampler, o._hasMipMaps);
        }
    };
}

namespace std
{
    template<>
    struct hash<::minire::rasterizer::textures::Id>
    {
        size_t operator()(::minire::rasterizer::textures::Id const & v) const
        {
            size_t result = 0;
            boost::hash_combine(result, std::hash<minire::content::Id>{}(v._contentId));
            boost::hash_combine(result, std::hash<minire::models::Sampler>{}(v._sampler));
            boost::hash_combine(result, std::hash<bool>{}(v._hasMipMaps));
            return result;
        }
    };
}
