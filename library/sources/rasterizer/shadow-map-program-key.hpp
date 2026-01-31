#pragma once

#include <minire/material.hpp>
#include <minire/models/mesh-features.hpp>

#include <boost/container_hash/hash.hpp> // for hash_combine

#include <cassert>
#include <functional> // For std::hash
#include <tuple>

namespace minire::rasterizer
{
    struct ShadowMapProgramKey
    {
        explicit ShadowMapProgramKey(models::MeshFeatures const & meshFeatures,
                                     material::Program::Locations const & attribLocations)
            : _vertexLocation(attribLocations._vertexAttribute)
            , _jointsLocation(attribLocations._jointsAttribute)
            , _weightsLocation(attribLocations._weightsAttribute)
        {
            assert(_vertexLocation >= 0);
            assert(!meshFeatures.hasSkin() || (_jointsLocation >= 0 && _weightsLocation >= 0));
            (void)meshFeatures;
        }

        bool operator==(ShadowMapProgramKey const & o) const
        {
            return std::tie(  _vertexLocation,   _jointsLocation,   _weightsLocation)
                == std::tie(o._vertexLocation, o._jointsLocation, o._weightsLocation);
        }

        bool hasSkin() const
        {
            assert((_jointsLocation >= 0) == (_weightsLocation >= 0));
            return _jointsLocation >= 0;
        }

        int const _vertexLocation = -1;
        int const _jointsLocation = -1;
        int const _weightsLocation = -1;
    };
}

namespace std
{
    template<>
    struct hash<::minire::rasterizer::ShadowMapProgramKey>
    {
        size_t operator()(::minire::rasterizer::ShadowMapProgramKey const & programKey) const
        {
            std::size_t result = 0;
            boost::hash_combine(result, programKey._vertexLocation);
            boost::hash_combine(result, programKey._jointsLocation);
            boost::hash_combine(result, programKey._weightsLocation);
            return result;
        }
    };
}