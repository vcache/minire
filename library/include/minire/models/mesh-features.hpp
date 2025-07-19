#pragma once

#include <functional> // For std::hash

#include <boost/container_hash/hash.hpp> // for hash_combine

namespace minire::models
{
    class MeshFeatures
    {
    public:
        explicit MeshFeatures(bool const hasUv,
                              bool const hasNormal,
                              bool const hasTangent)
            : _hasUv(hasUv)
            , _hasNormal(hasNormal)
            , _hasTangent(hasTangent)
        {}

    public:
        bool hasUv() const      { return _hasUv; }
        bool hasNormal() const  { return _hasNormal; }
        bool hasTangent() const { return _hasTangent; }

    public:
        bool operator==(MeshFeatures const & o) const
        {
            return _hasUv == o._hasUv
                && _hasNormal == o._hasNormal
                && _hasTangent == o._hasTangent;
        }

    private:
        bool const _hasUv;
        bool const _hasNormal;
        bool const _hasTangent;
    };
}

namespace std
{
    template<>
    struct hash<::minire::models::MeshFeatures>
    {
        size_t operator()(::minire::models::MeshFeatures const & v) const
        {
            size_t result = 0x906CDE457AEBF354ULL;
            boost::hash_combine(result, std::hash<bool>{}(v.hasUv()));
            boost::hash_combine(result, std::hash<bool>{}(v.hasNormal()));
            boost::hash_combine(result, std::hash<bool>{}(v.hasTangent()));
            return result;
        }
    };
}