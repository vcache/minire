#pragma once

#include <minire/utils/no-value.hpp>

#include <cassert>
#include <limits>
#include <memory>
#include <string>

// implementation details to ignored
namespace minire { class SceneImpl; }

namespace minire::utils
{
    class ObjectBase
    {
    public:
        virtual ~ObjectBase() = default;

    protected:
        using Mask = size_t;

        static constexpr Mask kNoFlags = 0;
        static constexpr Mask kAllFlags = std::numeric_limits<Mask>::max();

        virtual bool invalidated() const = 0;

        virtual void invalidate(Mask mask = kAllFlags) = 0;
        virtual void revalidate(Mask mask = kAllFlags) = 0;

    private:
        static constexpr size_t kNoIndex = kNoValue<size_t>;
        size_t _activationIndex = kNoIndex;

        friend class ::minire::SceneImpl;
    };

    template<typename Derived,
             typename Model,
             bool kImmutable = false>
    class Object
        : public ObjectBase
    {
    public:
        using Sptr = std::shared_ptr<Derived>;
        using Wptr = std::weak_ptr<Derived>;
        using ModelType = Model;

    public:
        std::string const & name() const { return _name; }

    public:
        Model const & model() const
        {
            return _model;
        }

        // NOTE: must not be called from ctor (due to a call to virtual invalidate())
        template<typename T>
        void setModel(T && model)
        {
            static_assert(!kImmutable, "cannot setModel for an immutable Object "
                                       "(but particular fields can be altered)");
            _model = std::forward<T>(model);
            invalidate();
        }

    protected:
        explicit Object(std::string name,
                        Model model,
                        Mask initial)
            : _name(name)
            , _model(std::move(model))
            , _flags(initial)
        {}

    protected:
        // NOTE: Descendants should also call Object::revalidate()
        void revalidate(Mask mask = kAllFlags) override { _flags &= ~mask; }

        static constexpr Mask mkMask(size_t index)
        {
            static_assert(sizeof(Mask) == 8, "unexpected size of size_t");
            static_assert(std::numeric_limits<Mask>::radix == 2,
                          "std::numeric_limits<T>::digits is in unexpected radix");
            assert(index < std::numeric_limits<Mask>::digits);
            return (1ULL << index);
        }

        // TODO: maybe change semantics from invalidate/revalidate to set/clear?

        bool invalidated() const override { return _flags != kNoFlags; }

        bool invalidatedAny(Mask mask) const { return kNoFlags != (_flags & mask); }
        bool invalidatedAll(Mask mask) const { return mask == (_flags & mask); }

        void invalidate(Mask mask = kAllFlags) override { _flags |= mask; }

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
