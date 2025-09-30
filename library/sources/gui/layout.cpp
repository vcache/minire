#include <minire/gui/layout.hpp>

#include <minire/errors.hpp>
#include <minire/gui/components/container.hpp>

namespace minire::gui
{
    void Layout::notify()
    {
        if (_parent)
        {
            _parent->rearrange();
        }
    }

    void Layout::setParent(components::Container & parent)
    {
        MINIRE_INVARIANT(!_parent, "a parent Container of a Layout cannot be changed");
        _parent = &parent;
    }
}