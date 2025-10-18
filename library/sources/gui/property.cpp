#include <minire/gui/property.hpp>

#include <minire/gui/component.hpp>

namespace minire::gui
{
    void PropertyBase::invalidate()
    {
        _invalidated = true;
        _owner.invalidate();
    }
}