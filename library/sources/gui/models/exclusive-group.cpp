#include <minire/gui/models/exclusive-group.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <cassert>

namespace minire::gui::models
{
    void ExclusiveGroup::add(Checkable * item)
    {
        _store.emplace(item);
        if (!_allowUnselect && !_selected)
            select(item);
    }

    void ExclusiveGroup::erase(Checkable * item)
    {
        bool reselect = item == _selected && !_allowUnselect;
        _store.erase(item);
        if (reselect)
        {
            select(_store.empty() ? nullptr : *_store.begin());
        }
    }

    void ExclusiveGroup::setAllowUnselect(bool allowUnselect)
    {
        if (allowUnselect == _allowUnselect)
            return;

        _allowUnselect = allowUnselect;

        if (!_allowUnselect && !_selected && !_store.empty())
            select(*_store.begin());
    }

    void ExclusiveGroup::unselect()
    {
        if (!_allowUnselect)
        {
            MINIRE_WARNING("attempted to unselect exclusive group in which ",
                           "unselection isn't allowed");
            return;
        }
        select(nullptr);
    }

    void ExclusiveGroup::select(Checkable * item)
    {
        MINIRE_INVARIANT(!item || _store.contains(item),
                         "cannot select a Checkable not from an ExclusiveGroup");

        if (item == _selected)
            return;

        Checkable * previous = _selected;
        if (previous)
        {
            previous->setCheckedImpl(false);
        }

        _selected = item;

        if (_selected)
        {
            _selected->setCheckedImpl(true);
        }

        if (_changeCallback)
            _changeCallback(previous, _selected);
    }
}