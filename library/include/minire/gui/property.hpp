#pragma once

#include <type_traits>
#include <utility>

namespace minire::gui
{
    class Component;

    class PropertyBase
    {
    public:
        bool isInvalidated() const { return _invalidated; }

        void revalidate() { _invalidated = false; }

    protected:
        explicit PropertyBase(Component & owner)
            : _owner(owner)
            , _invalidated(true)
        {}

        void invalidate();

    private:
        Component & _owner;
        bool        _invalidated;
    };

    template<typename Value>
    class Property
        : public PropertyBase
    {
        template<typename T>
        constexpr static bool kIsProperty = std::is_same_v<std::decay_t<T>,
                                                           std::decay_t<Property<Value>>>;

    public:
        explicit Property(Component & owner)
            : PropertyBase(owner)
            , _value(Value())
        {}

        template<typename T>
        explicit Property(Component & owner,
                          T && value)
            : PropertyBase(owner)
            , _value(std::forward<T>(value))
        {}

    public:
        Value const & get() const { return _value; }

        Value const & operator*() const { return get(); }

        Value const * operator->() const { return &get(); }

    public:
        template<typename T,
                 typename = std::enable_if_t<!kIsProperty<T>>>
        void set(T && value)
        {
            if (_value != value)
            {
                _value = std::forward<T>(value);
                invalidate();
            }
        }

        void set(Property<Value> const & newValue)
        {
            set(newValue.get());
        }

        Value & operator*()
        {
            invalidate();
            return _value;
        }

        Value * operator->() { return &(operator *()); }

        template<typename T,
                 typename = std::enable_if_t<!kIsProperty<T>>>
        Property & operator=(T && newValue)
        {
            set(std::forward<T>(newValue));
            return *this;
        }

        Property & operator=(Property<Value> const & newValue)
        {
            set(newValue.get());
            return *this;
        }

        template<typename Functor>
        void edit(Functor functor)
        {
            if (functor(_value))
            {
                invalidate();
            }
        }

    private:
        Value _value;
    };
}