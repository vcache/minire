#pragma once

#include <minire/content/path.hpp>
#include <minire/material.hpp>

#include <boost/container_hash/hash.hpp> // for hash_combine

#include <functional> // For std::hash

namespace minire::rasterizer::meshes
{
    struct Id
    {
        content::Path         _contentPath;
        material::Model::Sptr _defaultMaterial;

        bool operator==(Id const & o) const
        {
            return std::tie(  _contentPath,   _defaultMaterial)
                == std::tie(o._contentPath, o._defaultMaterial);
        }
    };
}

namespace std
{
    template<>
    struct hash<::minire::rasterizer::meshes::Id>
    {
        size_t operator()(::minire::rasterizer::meshes::Id const & v) const
        {
            size_t result = 0;
            boost::hash_combine(result, std::hash<minire::content::Path>{}(v._contentPath));
            boost::hash_combine(result, std::hash<minire::material::Model::Sptr>{}(v._defaultMaterial));
            return result;
        }
    };
}
