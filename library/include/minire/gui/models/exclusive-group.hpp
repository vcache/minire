#pragma once

#include <minire/gui/models/checkable.hpp>

#include <functional>
#include <limits>
#include <memory>
#include <unordered_set>

namespace minire::gui::models
{
    class ExclusiveGroup
    {
    public:
        explicit ExclusiveGroup(bool allowUnselect = false)
            : _allowUnselect(allowUnselect)
        {}

        template<typename Callback>
        explicit ExclusiveGroup(bool allowUnselect,
                                Callback callback)
            : _changeCallback(callback)
            , _allowUnselect(allowUnselect)
        {}

        void select(Checkable *);

        void select(Checkable & item) { select(&item); }

        void unselect();

        Checkable * selected() const { return _selected; }

    public:
        void setAllowUnselect(bool v);

        bool allowUnselect() const { return _allowUnselect; }

    public:
        template<typename T>
        void setChangeCallback(T changeCallback)
        {
            _changeCallback = changeCallback;
        }

    private:
        void add(Checkable *);

        void erase(Checkable *);

    private:
        using ChangeCallback = std::function<void(Checkable * previous,
                                                  Checkable * current)>;
        using Store = std::unordered_set<Checkable *>;

        Store          _store;
        ChangeCallback _changeCallback;
        Checkable    * _selected = nullptr;
        bool           _allowUnselect = true;

        friend class Checkable;
    };
}