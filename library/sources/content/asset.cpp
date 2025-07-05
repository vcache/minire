#include <minire/content/asset.hpp>

#include <minire/utils/demangle.hpp>

#include <type_traits>

namespace minire::content
{
    std::string demangle(Asset const & asset)
    {
        return std::visit(
            [](auto const & item)
            {
                using T = std::decay_t<decltype(item)>;
                return utils::demangle<T>();
            },
            asset);
    }
}
