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

    // TODO: implement it
    size_t sizeOf(Asset const &)
    {
        return 0;
    }

    bool hasData(Asset const & asset)
    {
        return !std::holds_alternative<std::monostate>(asset);
    }
}
