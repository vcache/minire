#pragma once

#include <minire/content/id.hpp>
#include <minire/models/sampler.hpp>

#include <boost/container_hash/hash.hpp> // for hash_combine

#include <functional> // For std::hash

namespace minire::rasterizer::textures
{
    struct Id
    {
        content::Id     const _contentId;
        models::Sampler const _sampler;
        bool            const _hasMipMaps;
        size_t          const _hash;

        explicit Id(content::Id contentId,
                    models::Sampler sampler,
                    bool hasMipMaps)
            : _contentId(std::move(contentId))
            , _sampler(std::move(sampler))
            , _hasMipMaps(hasMipMaps)
            , _hash([this]()
            {
                size_t result = 0;
                boost::hash_combine(result, std::hash<minire::content::Id>{}(_contentId));
                boost::hash_combine(result, std::hash<minire::models::Sampler>{}(_sampler));
                boost::hash_combine(result, std::hash<bool>{}(_hasMipMaps));
                return result;
            }())
        {}

        bool operator==(Id const & o) const
        {
            if (_hash != o._hash) return false;

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
            return v._hash;
        }
    };
}
