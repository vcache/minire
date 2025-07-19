#pragma once

#include <functional> // For std::hash

#include <boost/container_hash/hash.hpp> // for hash_combine

namespace minire::models
{
    struct Sampler
    {
        // TODO: shouldn't hardcode these values

        int _minFilter = 9987;  // GL_LINEAR_MIPMAP_LINEAR;
        int _magFilter = 9729;  // GL_LINEAR;
        int _wrapS = 10497;     // GL_REPEAT;
        int _wrapT = 10497;     // GL_REPEAT;

        bool operator==(Sampler const & o) const
        {
            return _minFilter == o._minFilter
                && _magFilter == o._magFilter
                && _wrapS == o._wrapS
                && _wrapT == o._wrapT;
        }
    };
}

namespace std
{
    template<>
    struct hash<::minire::models::Sampler>
    {
        size_t operator()(::minire::models::Sampler const & sampler) const
        {
            size_t result = 0x984AB3010DE4AB71ULL;
            boost::hash_combine(result, std::hash<int>{}(sampler._minFilter));
            boost::hash_combine(result, std::hash<int>{}(sampler._magFilter));
            boost::hash_combine(result, std::hash<int>{}(sampler._wrapS));
            boost::hash_combine(result, std::hash<int>{}(sampler._wrapT));
            return result;
        }
    };
}