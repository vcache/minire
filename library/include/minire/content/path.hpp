#pragma once

#include <minire/content/id.hpp>

#include <fmt/format.h>

#include <cstddef>
#include <functional> // For std::hash
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace minire::content
{
    namespace path
    {
        enum class Special
        {
            kCameras,
            kLights,
            kMeshes,
            kNodes,
            kScenes,
            kVertexBuffers,
        };

        using Index = size_t;

        using Component = std::variant<Id, Index, Special>;
    }

    using Path = std::vector<path::Component>;

    std::string toString(Path const &);
    size_t hash(Path const &);

    template<typename... Args>
    Path mkPath(Args && ... args)
    {
        return Path{ {path::Component(std::forward<Args>(args))...} };
    }

    Path concat(Path const & prefix, path::Component suffix);
}

namespace std
{
    template<>
    struct hash<::minire::content::Path>
    {
        size_t operator()(::minire::content::Path const & path) const
        {
            return ::minire::content::hash(path);
        }
    };
}

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_same_v<T, ::minire::content::Path>, char>>
    : fmt::formatter<std::string>
{
    template <typename FormatCtx>
    auto format(T const & value, FormatCtx & ctx) const
    {
        return fmt::formatter<std::string>::format(::minire::content::toString(value), ctx);
    }
};
