#pragma once

#include <boost/container_hash/hash.hpp> // for hash_combine
#include <fmt/format.h>

#include <functional> // For std::hash
#include <tuple>

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

    public:
        std::string toString() const
        {
            return fmt::format("(hasUv = {}, hasNormal = {}, hasTangent = {}, hasSkin = {})",
                               _hasUv, _hasNormal, _hasTangent, _hasSkin);
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
            size_t result = 0;
            boost::hash_combine(result, v.hasUv());
            boost::hash_combine(result, v.hasNormal());
            boost::hash_combine(result, v.hasTangent());
            boost::hash_combine(result, v.hasSkin());
            return result;
        }
    };
}

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_same_v<T, ::minire::models::MeshFeatures>, char>>
    : fmt::formatter<std::string>
{
    template <typename FormatCtx>
    auto format(T const & value, FormatCtx & ctx) const
    {
        return fmt::formatter<std::string>::format(value.toString(), ctx);
    }
};
