#include <minire/content/path.hpp>

#include <minire/errors.hpp>

#include <utils/overloaded.hpp>

#include <boost/container_hash/hash.hpp> // for hash_combine
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace minire::content
{
    namespace
    {
        std::string toString(path::Special special)
        {
            switch(special)
            {
                case path::Special::kCameras: return "cameras";
                case path::Special::kLights:  return "lights";
                case path::Special::kMeshes:  return "meshes";
                case path::Special::kNodes:   return "nodes";
                case path::Special::kScenes:  return "scenes";
            }
            MINIRE_THROW("unexpected special path component: {}",
                         static_cast<int>(special));
        }
    }

    std::string toString(Path const & path)
    {
        std::vector<std::string> components;
        components.reserve(path.size());

        for(path::Component const & component : path)
        {
            components.emplace_back(std::visit(
                utils::Overloaded
                {
                    [](Id const & v)    { return v; },
                    [](path::Index v)   { return std::to_string(v); },
                    [](path::Special v) { return toString(v); },
                },
                component
            ));
        }

        return fmt::format("{}", fmt::join(components, "/"));
    }

    size_t hash(Path const & path)
    {
        size_t result = 0x96810CDEA74F298AULL;
        for(path::Component const & component : path)
        {
            size_t componentHash = std::visit(
                utils::Overloaded
                {
                    [](Id const & v)    { return std::hash<Id>{}(v); },
                    [](path::Index v)   { return std::hash<path::Index>{}(v); },
                    [](path::Special v) { return std::hash<size_t>{}(static_cast<size_t>(v)); },
                }, component);
            boost::hash_combine(result, componentHash);
        }
        return result;
    }

    Path concat(Path const & prefix, path::Component suffix)
    {
        Path result = prefix;
        result.emplace_back(std::move(suffix));
        return result;
    }
}
