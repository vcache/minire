#include <minire/gui/models/checkable.hpp>

#include <minire/gui/models/exclusive-group.hpp>

namespace minire::gui::models
{
    Checkable::Checkable(bool checkable)
        : _checkable(checkable)
    {}

    Checkable::~Checkable()
    {
        setExclusiveGroup({});
    }

    void Checkable::setCheckable(bool v)
    {
        _checkable = v;
        if (!_checkable)
            _checked = false;
    }

    void Checkable::setChecked(bool checked)
    {
        if (!_checkable || _checked == checked)
            return;

        if (!_exclusiveGroup)
        {
            setCheckedImpl(checked);
        }
        else
        {
            if (checked)
            {
                _exclusiveGroup->select(this);
            }
            else
            {
                if (_exclusiveGroup->allowUnselect() &&
                    _exclusiveGroup->selected() == this)
                {
                    _exclusiveGroup->unselect();
                }
            }
        }
    }

    bool Checkable::canUncheck() const
    {
        return !_exclusiveGroup ||
                _exclusiveGroup->allowUnselect();
    }

    void Checkable::setExclusiveGroup(ExclusiveGroupSptr const & newExclusiveGroup)
    {
        if (_exclusiveGroup)
        {
            _exclusiveGroup->erase(this);
        }

        _exclusiveGroup = newExclusiveGroup;

        if (_exclusiveGroup)
        {
            _exclusiveGroup->add(this);
        }
    }

    void Checkable::setCheckedImpl(bool checked)
    {
        _checked = checked;
        onCheckChanged();
        if (_checkedCallback)
        {
            _checkedCallback(*this);
        }
    }
}