#include <minire/models/node-pointer.hpp>

#include <minire/utils/overloaded.hpp>
#include <minire/scene.hpp> // for scene::Node

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace minire::models
{
    std::string toString(NodePointer const & nodePointer)
    {
        return std::visit(utils::Overloaded
        {
            [](ScenePath const & p)
            {
                return fmt::format("NodePointer(ScenePath, {})", p);
            },
            [](scene::Node::Sptr const & p)
            {
                return fmt::format("NodePointer(Node::Sptr, {})",
                                   (void const *) p.get());
            },
        }, nodePointer);
    }
}
