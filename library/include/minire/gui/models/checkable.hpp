#pragma once

#include <minire/gui/callbacks.hpp>

#include <memory>

namespace minire::gui::models
{
    namespace checkable
    {
        struct OnCheckedChanged
        {
            bool _checked = false;
        };
    }

    class ExclusiveGroup;

    class Checkable
        : public Callback<Checkable, checkable::OnCheckedChanged>
    {
    public:
        explicit Checkable(bool checkable = false);

        virtual ~Checkable();

        void setCheckable(bool v);

        bool checkable() const { return _checkable; }

    public:
        void setChecked(bool checked);

        bool checked() const { return _checked; }

        void toggleCheck() { setChecked(!_checked); }

        bool canUncheck() const;

    public:
        using ExclusiveGroupSptr = std::shared_ptr<ExclusiveGroup>;
        void setExclusiveGroup(ExclusiveGroupSptr const &);

    private:
        void setCheckedImpl(bool checked);

    private:
        ExclusiveGroupSptr _exclusiveGroup;
        bool               _checkable = false;
        bool               _checked = false;

        friend class ExclusiveGroup;
    };
}
