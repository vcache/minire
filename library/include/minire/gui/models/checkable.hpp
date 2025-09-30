#pragma once

#include <functional>

namespace minire::gui::models
{
    class Checkable
    {
    public:
        explicit Checkable(bool checkable = false)
            : _checkable(checkable)
        {}

        virtual ~Checkable() = default;

        void setCheckable(bool v) { _checkable = v; }

        bool checkable() const { return _checkable; }

    public:
        void setChecked(bool checked)
        {
            if (!_checkable || _checked == checked)
                return;

            _checked = checked;
            onCheckChanged();
        }

        bool checked() const { return _checked; }

        void toggleCheck() { setChecked(!_checked); }

    public:
        template<typename T>
        void setCheckedCallback(T checkedCallback)
        {
            _checkedCallback = checkedCallback;
        }

    protected:
        virtual void onCheckChanged()
        {
            if (_checkedCallback)
                _checkedCallback(*this);
        }

    private:
        using CheckedCallback = std::function<void(Checkable &)>;

        CheckedCallback _checkedCallback;
        bool            _checkable = false;
        bool            _checked = false;
    };
}