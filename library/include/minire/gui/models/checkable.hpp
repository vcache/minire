#pragma once

#include <functional>
#include <memory>

namespace minire::gui::models
{
    class ExclusiveGroup;

    class Checkable
    {
    public:
        using ExclusiveGroupSptr = std::shared_ptr<ExclusiveGroup>;

        explicit Checkable(bool checkable = false,
                           ExclusiveGroupSptr const & = {});

        virtual ~Checkable();

        void setCheckable(bool v);

        bool checkable() const { return _checkable; }

    public:
        void setChecked(bool checked);

        bool checked() const { return _checked; }

        void toggleCheck() { setChecked(!_checked); }

        bool canUncheck() const;

    public:
        template<typename T>
        void setCheckedCallback(T checkedCallback)
        {
            _checkedCallback = checkedCallback;
        }

    public:
        void setExclusiveGroup(ExclusiveGroupSptr const &);

    protected:
        virtual void onCheckChanged() {}

    private:
        void setCheckedImpl(bool checked);

    private:
        using CheckedCallback = std::function<void(Checkable &)>;

        CheckedCallback    _checkedCallback;
        ExclusiveGroupSptr _exclusiveGroup;
        bool               _checkable = false;
        bool               _checked = false;

        friend class ExclusiveGroup;
    };
}