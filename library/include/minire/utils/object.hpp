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
        using Mask = size_t;
        static constexpr Mask kAllFlags = std::numeric_limits<Mask>::max();

    public:
        using Sptr = std::shared_ptr<Derived>;
        using Wptr = std::weak_ptr<Derived>;
        using ModelType = Model;

        explicit Object(std::string name,
                        Model model)
            : _name(name)
            , _model(std::move(model))
            , _flags(0)
            , _detached(false)
        {}

        virtual ~Object() = default;

    public:
        virtual void detach()
        {
            _detached = true;
        }

        // If true, the detaching is pended and will be performed
        // during the next rendering iteration.
        bool detached() const
        {
            return _detached;
        }

        std::string const & name() const { return _name; }

    public:
        Model const & model() const
        {
            return _model;
        }

        template<typename T>
        void setModel(T && model)
        {
            static_assert(!kImmutable, "cannot setModel for an immutable Object");
            _model = std::forward<T>(model);
            invalidate();
        }

        // NOTE: Descendants should also call Object::revalidate()
        virtual void revalidate() { _flags = 0; }

    protected:
        static constexpr Mask mkMask(size_t index)
        {
            static_assert(sizeof(Mask) == 8, "unexpected size of size_t");
            assert(index < 64);
            return (1ULL << index);
        }

        bool invalidated() const { return _flags != 0; }
        bool invalidated(Mask mask) const { return _flags & mask; }

        // NOTE: since "invalidate" calls a virtual methods,
        //       it shouldn't be called from a constructor.

        void invalidate()
        {
            bool wasInvalidated = invalidated();
            _flags = kAllFlags;
            if (_allowPropagation && !wasInvalidated) propagate();
        }

        void invalidate(Mask mask)
        {
            bool wasInvalidated = invalidated();
            _flags |= mask;
            if (_allowPropagation && !wasInvalidated) propagate();
        }

        void setAllowPropagation(bool v) { _allowPropagation = v; }

        // called once any flag set, but just once
        virtual void propagate() {}

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
        bool              _detached = false;
        bool              _allowPropagation = false; // TODO: get rid of this hack
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