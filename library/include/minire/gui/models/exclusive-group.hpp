#pragma once

#include <minire/gui/callbacks.hpp>
#include <minire/gui/models/checkable.hpp>

#include <functional>
#include <limits>
#include <memory>
#include <unordered_set>

namespace minire::gui::models
{
    namespace exclusive_group
    {
        struct OnChange
        {
            Checkable * _previous = nullptr;
            Checkable * _current = nullptr;
        };
    }

    class ExclusiveGroup
        : public Callback<ExclusiveGroup, exclusive_group::OnChange>
    {
    public:
        explicit ExclusiveGroup(bool allowUnselect = false)
            : _allowUnselect(allowUnselect)
        {}

        void select(Checkable *);

        void select(Checkable & item) { select(&item); }

        void unselect();

        Checkable * selected() const { return _selected; }

    public:
        void setAllowUnselect(bool v);

        bool allowUnselect() const { return _allowUnselect; }

    private:
        void add(Checkable *);

        void erase(Checkable *);

    private:
        using Store = std::unordered_set<Checkable *>;

        Store          _store;
        Checkable    * _selected = nullptr;
        bool           _allowUnselect = true;

        friend class Checkable;
    };
}
