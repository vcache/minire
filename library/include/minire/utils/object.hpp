#pragma once

#include <cassert>
#include <limits>
#include <memory>
#include <string>

namespace minire::utils
{
    template<typename Derived,
             typename Model,
             bool kImmutable = false>
    class Object
    {
    public:
        using Sptr = std::shared_ptr<Derived>;
        using Wptr = std::weak_ptr<Derived>;
        using ModelType = Model;

        explicit Object(std::string name,
                        Model model)
            : _name(name)
            , _model(std::move(model))
            , _flags(0)
        {}

        virtual ~Object() = default;

    public:
        std::string const & name() const { return _name; }

    public:
        Model const & model() const
        {
            return _model;
        }

        template<typename T>
        void setModel(T && model)
        {
            static_assert(!kImmutable, "cannot setModel for an immutable Object "
                                       "(but particular fields can be altered)");
            _model = std::forward<T>(model);
            invalidate();
        }

    protected:
        using Mask = size_t;
        static constexpr Mask kAllFlags = std::numeric_limits<Mask>::max();

        // NOTE: Descendants should also call Object::revalidate()
        virtual void revalidate(Mask mask = kAllFlags) { _flags &= ~mask; }

        static constexpr Mask mkMask(size_t index)
        {
            static_assert(sizeof(Mask) == 8, "unexpected size of size_t");
            static_assert(std::numeric_limits<Mask>::radix == 2,
                          "std::numeric_limits<T>::digits is in unexpected radix");
            assert(index < std::numeric_limits<Mask>::digits);
            return (1ULL << index);
        }

        // TODO: maybe change semantics from invalidate/revalidate to set/clear?

        bool invalidated() const { return _flags != 0; }

        bool invalidatedAny(Mask mask) const { return 0 != (_flags & mask); }
        bool invalidatedAll(Mask mask) const { return mask == (_flags & mask); }

        // NOTE: since "invalidate" calls a virtual methods,
        //       it shouldn't be called from a constructor.

        void invalidate()
        {
            _flags = kAllFlags;
            if (_allowPropagation) propagate(kAllFlags);
        }

        void invalidate(Mask mask, bool allowPropagation = true)
        {
            _flags |= mask;
            if (_allowPropagation && allowPropagation && mask) propagate(mask);
        }

        void setAllowPropagation(bool v) { _allowPropagation = v; }

        // called only for a newly set flags only
        virtual void propagate(Mask) {}

        void propagate() { propagate(_flags); }

    protected:
        Model & model(Mask mask)
        {
            invalidate(mask);
            return _model;
        }

    private:
        std::string const _name;
        Model             _model;
        Mask              _flags = 0;
        bool              _allowPropagation = false; // TODO: get rid of this hack
    };

    // A helper for detachable-objects
    class Detachable
    {
    public:
        void detach()
        {
            _detached = true;
        }

        // If true, the detaching is pended and will be performed
        // during the next rendering iteration.
        bool detached() const
        {
            return _detached;
        }

    private:
        bool _detached = false;
    };

    // A helper for RAII-style lifecycle of an Object
    template<typename T>
    class ObjectGuard
    {
        ObjectGuard(ObjectGuard const &) = delete;
        ObjectGuard& operator=(ObjectGuard const &) = delete;
        ObjectGuard(ObjectGuard &&) = delete;
        ObjectGuard& operator=(ObjectGuard &&) = delete;

    public:
        explicit ObjectGuard(typename T::Sptr const & object)
            : _object(object)
        {}

        ~ObjectGuard()
        {
            if (_object)
            {
                _object->detach();
            }
        }

        operator bool() const { return _object.operator bool(); }

        auto const & operator*() const { assert(_object); return *_object.get(); }
        auto & operator*() { assert(_object); return *_object.get(); }

        auto const * operator->() const { assert(_object); return _object.get(); }
        auto * operator->() { assert(_object); return _object.get(); }

    private:
        typename T::Sptr _object;
    };
}
