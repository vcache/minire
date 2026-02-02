#pragma once

#include <boost/container_hash/hash.hpp> // for hash_combine
#include <minire/material.hpp>

#include <functional> // For std::hash
#include <string>
#include <unordered_map>

namespace minire::codegen
{
    struct Traits
    {
        material::Program::Locations _attribLocations;

        bool operator==(Traits const & other) const
        {
            return _attribLocations == other._attribLocations;
        }
    };
}

namespace std
{
    template<>
    struct hash<::minire::codegen::Traits>
    {
        size_t operator()(::minire::codegen::Traits const & traits) const
        {
            std::size_t result = 0;
            boost::hash_combine(result,
                std::hash<::minire::material::Program::Locations>{}(traits._attribLocations));
            return result;
        }
    };
}
