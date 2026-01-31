#include <minire/models/scene-path.hpp>

#include <minire/errors.hpp>

#include <fmt/ranges.h>

namespace minire::models
{
    ScenePath concat(ScenePath const & a, ScenePath const & b)
    {
        ScenePath result = a;
        result.insert(result.end(), b.begin(), b.end());
        return result;
    }

    ScenePath concat(ScenePath const & a, std::string const & b)
    {
        ScenePath result = a;
        result.emplace_back(b);
        return result;
    }

    ScenePath cutPrefix(ScenePath const & full, ScenePath const & prefix)
    {
        MINIRE_INVARIANT(full.size() >= prefix.size(),
                         "prefix is longer than full string: full=\"{}\" prefix=\"{}\"",
                         full, prefix);
        for(size_t i = 0; i < prefix.size(); ++i)
        {
            MINIRE_INVARIANT(full[i] == prefix[i],
                             "prefix isn't a prefix (at {}): full=\"{}\" prefix=\"{}\"",
                             i, full, prefix);
        }
        return ScenePath(full.begin() + prefix.size(), full.end());
    }
}
