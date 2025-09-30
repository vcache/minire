#pragma once

#include <minire/gui/area.hpp>

#include <memory>

namespace minire::gui::components { class Container; }

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

        virtual void onErase(Component const &) {}

        virtual void onClear() {}

    protected:
        void notify();

    private:
        void setParent(components::Container &);

    private:
        // NOTE: this has semantics of a reference,
        //       but have to use raw pointer because
        //       cannot access to Component::Sptr inside
        //       Component's ctor;
        components::Container * _parent = nullptr;

        friend class components::Container;
    };
}
