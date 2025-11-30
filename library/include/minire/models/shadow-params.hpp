#pragma once

#include <optional>

namespace minire::models
{
    struct ShadowParams
    {
        size_t _mapSize = 1024;
        bool   _usePCF = false;

        // TODO: implement shadow groups (maybe it should be a bitset?)
    };

    using MaybeShadowParams = std::optional<ShadowParams>;
}