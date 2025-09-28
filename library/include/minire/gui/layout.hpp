#pragma once

#include <minire/gui/area.hpp>

#include <memory>

namespace minire::gui
{
    class Component;
    class Layout
    {
    public:
        using Sptr = std::shared_ptr<Layout>;

        virtual ~Layout() = default;

        virtual Area evaluate(Area const & client,
                              Component const &) const
        {
            return client;
        }
    };
}
