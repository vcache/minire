#pragma once

#include <minire/models/scene-path.hpp>

#include <fmt/format.h>

#include <string>
#include <variant>

namespace minire::scene { class Node; }

namespace minire::models
{
    // NOTE: the ScenePath is relative to a node where a NodePointer is applied,
    //       and can be empty (to affect a container node itself).
    using NodePointer = std::variant<ScenePath,
                                     std::shared_ptr<scene::Node>>;

    std::string toString(NodePointer const &);
}

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_same_v<T, minire::models::NodePointer>, char>>
    : fmt::formatter<std::string>
{
    template <typename FormatCtx>
    auto format(T const & value, FormatCtx & ctx) const
    {
        return fmt::formatter<std::string>::format(minire::models::toString(value), ctx);
    }
};
