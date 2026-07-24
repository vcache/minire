#pragma once

#include <minire/content/handle.hpp>
#include <minire/material.hpp>

#include <boost/container_hash/hash.hpp> // for hash_combine

#include <functional> // For std::hash

namespace minire::rasterizer::meshes
{
    struct Id
    {
        content::Handle const _contentPath;
        Material::Sptr  const _defaultMaterial;
        size_t          const _hash;

        explicit Id(content::Handle contentPath,
                    Material::Sptr const & defaultMaterial)
            : _contentPath(std::move(contentPath))
            , _defaultMaterial(defaultMaterial)
            , _hash([this]()
            {
                size_t result = 0;
                boost::hash_combine(result, std::hash<content::Handle>{}(_contentPath));
                boost::hash_combine(result, std::hash<Material::Sptr>{}(_defaultMaterial));
                return result;
            }())
        {}

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
            return v._hash;
        }
    };
}
