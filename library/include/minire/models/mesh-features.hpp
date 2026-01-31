#pragma once

#include <functional> // For std::hash
#include <tuple>

#include <boost/container_hash/hash.hpp> // for hash_combine

namespace minire::models
{
    class MeshFeatures
    {
    public:
        explicit MeshFeatures(bool const hasUv,
                              bool const hasNormal,
                              bool const hasTangent,
                              bool const hasSkin)
            : _hasUv(hasUv)
            , _hasNormal(hasNormal)
            , _hasTangent(hasTangent)
            , _hasSkin(hasSkin)
        {}

    public:
        bool hasUv() const      { return _hasUv; }
        bool hasNormal() const  { return _hasNormal; }
        bool hasTangent() const { return _hasTangent; }
        bool hasSkin() const    { return _hasSkin; }

    public:
        bool operator==(MeshFeatures const & o) const
        {
            return std::tie(  _hasUv,   _hasNormal,   _hasTangent,   _hasSkin)
                == std::tie(o._hasUv, o._hasNormal, o._hasTangent, o._hasSkin);
        }

    private:
        bool _hasUv;
        bool _hasNormal;
        bool _hasTangent;
        bool _hasSkin;  // joints+weights
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
            boost::hash_combine(result, v.hasUv());
            boost::hash_combine(result, v.hasNormal());
            boost::hash_combine(result, v.hasTangent());
            boost::hash_combine(result, v.hasSkin());
            return result;
        }
    };
}
