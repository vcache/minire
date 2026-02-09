#pragma once

#include <minire/gui/area.hpp>

#include <memory>
#include <vector>

namespace minire::gui
{
    class Component;

    class Layout
    {
    public:
        using Sptr = std::shared_ptr<Layout>;

        virtual ~Layout() = default;

        virtual void onInsert(Component const &) {}

        virtual void onErase(Component const &) {}

        virtual void onClear() {}

    public:
        struct Target
        {
            Component const & _component;
        };

        using Targets = std::vector<Target>;
        using Areas = std::vector<Area>;

        virtual Areas evaluate(Area const & clientArea,
                               Targets const &) const = 0;
    };

    class LinearLayout
        :  public Layout
    {
    public:
        virtual Area evaluate(Area const & client,
                              Component const &) const
        {
            return client;
        }

        Areas evaluate(Area const & clientArea,
                       Targets const & targets) const override
        {
            Areas result;
            result.reserve(targets.size());
            for(Target const & target : targets)
            {
                result.emplace_back(
                    evaluate(clientArea, target._component));
            }
            return result;
        }
    };
}
