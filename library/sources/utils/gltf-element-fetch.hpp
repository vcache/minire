#pragma once

#include <minire/content/path.hpp>
#include <minire/errors.hpp>
#include <minire/utils/demangle.hpp>
#include <minire/utils/overloaded.hpp>

#include <type_traits>
#include <vector>

namespace minire::utils
{
    template<typename T>
    size_t getElementIndex(content::path::Component const & sceneId,
                           std::vector<T> const & container,
                           char const * kind)
    {
        return std::visit(utils::Overloaded
            {
                [&container, kind](content::path::Index index) -> size_t
                {
                    MINIRE_INVARIANT(index < container.size(),
                         "bad {} index ({} >= {})",
                         kind, index, container.size());
                    return index;
                },
                [&container, kind](content::Id const & name) -> size_t
                {
                    for(size_t i = 0; i < container.size(); ++i)
                    {
                        if (container[i].name == name)
                            return i;
                    }
                    MINIRE_THROW("no such {}: \"{}\"", kind, name);
                },
                [](auto const & v) -> size_t
                {
                    using U = std::decay_t<decltype(v)>;
                    MINIRE_THROW("unexpected path component type (not a string or size_t): {}",
                                 utils::demangle<U>());
                }
            }, sceneId);
    }
}
