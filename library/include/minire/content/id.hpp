#pragma once

#include <optional>
#include <string>
#include <unordered_set>

namespace minire::content
{
    using Id = std::string;
    using MaybeId = std::optional<Id>;
    using Ids = std::unordered_set<Id>;
}
