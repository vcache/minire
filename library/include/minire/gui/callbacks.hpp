#pragma once

#include <minire/errors.hpp>
#include <minire/events/application.hpp>
#include <minire/utils/demangle.hpp>

#include <cassert>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace minire::gui
{
    namespace events
    {
        struct OnDragBegin
        {
            minire::events::application::OnMouseDown _event;
        };

        struct OnDragMove
        {
            minire::events::application::OnMouseDown _begin;
            minire::events::application::OnMouseMove _event;
        };

        struct OnDragEnd
        {
            std::optional<minire::events::application::OnMouseUp> _event;
        };

        struct OnMouseEnter
        {
            bool _isClickReturn;    // TODO: maybe use Drag logics instead?
        };

        struct OnMouseLeave {};

        struct OnFocus {};

        struct OnUnfocus {};

        struct OnClick {};
    }

    template<typename Derived, typename Event>
    class Callback
    {
    public:
        virtual void handle(Event const & event)
        {
            for(auto [_, callback] : _callbacks)
            {
                assert(callback);
                callback(dynamic_cast<Derived &>(*this), event);
            }
        }

        template<typename Callback>
        void setCallback(std::in_place_type_t<Event>,
                         std::string const & name,
                         Callback callback)
        {
            auto [_, inserted] = _callbacks.emplace(name, std::move(callback));
            MINIRE_INVARIANT(inserted, "a callback {} for \"{}\" is already set: \"{}\"",
                             utils::demangle<Event>(), utils::demangle<Callback>(), name);
        }

        void eraseCallback(std::in_place_type_t<Event>,
                           std::string const & name)
        {
            _callbacks.erase(name);
        }

    private:
        using Functor = std::function<void(Derived &, Event const &)>;
        using Callbacks = std::unordered_map<std::string, Functor>;

        Callbacks _callbacks;
    };

    template<typename Derived, typename... Ts>
    class CallbacksBuilder
        : public Callback<Derived, Ts>...
    {
    public:
        using Callback<Derived, Ts>::handle...;
        using Callback<Derived, Ts>::setCallback...;
        using Callback<Derived, Ts>::eraseCallback...;
    };

    template<typename Derived>
    class CommonCallbacks
        : public CallbacksBuilder<Derived,
                                  minire::events::application::OnMouseWheel,
                                  minire::events::application::OnMouseMove,
                                  minire::events::application::OnMouseDown,
                                  minire::events::application::OnMouseUp,
                                  minire::events::application::OnKeyUp,
                                  minire::events::application::OnKeyDown,
                                  minire::events::application::OnTextInput,
                                  minire::events::application::OnClipboardUpdate,
                                  gui::events::OnDragBegin,
                                  gui::events::OnDragMove,
                                  gui::events::OnDragEnd,
                                  gui::events::OnMouseEnter,
                                  gui::events::OnMouseLeave,
                                  gui::events::OnFocus,
                                  gui::events::OnUnfocus,
                                  gui::events::OnClick>
    {};

    // A RAII-like handler for a Callback.
    // Must be when a Callback captures objects that might become
    // dangling.

    class GenericCallbackHolder
    {
    public:
        using Uptr = std::unique_ptr<GenericCallbackHolder>;
        virtual ~GenericCallbackHolder() = default;
    };

    template<typename Owner,
             typename Event>
    class CallbackHolder final
        : public GenericCallbackHolder
    {
        CallbackHolder(CallbackHolder const &) = delete;
        CallbackHolder & operator=(CallbackHolder const &) = delete;

    public:
        explicit CallbackHolder(std::shared_ptr<Owner> const & owner,
                                std::string const & name)
            : _owner(owner)
            , _name(name)
        {}

        explicit CallbackHolder(CallbackHolder && other)
            : _owner(std::move(other._owner))
            , _name(std::move(other._name))
        {}

        CallbackHolder & operator=(CallbackHolder && other)
        {
            CallbackHolder tmp(std::move(other));
            std::swap(_owner, tmp._owner);
            std::swap(_name, tmp._name);
            return *this;
        }

        ~CallbackHolder()
        {
            if (std::shared_ptr<Owner> owner = _owner.lock(); owner)
            {
                owner->eraseCallback(std::in_place_type<Event>, _name);
            }
        }

    private:
        std::weak_ptr<Owner> _owner;
        std::string          _name;
    };

    template<typename Event,
             typename Owner,
             typename Callback>
    std::unique_ptr<CallbackHolder<Owner, Event>>
    registerCallback(std::shared_ptr<Owner> const & owner,
                     std::string const & name,
                     Callback callback)
    {
        if (owner)
        {
            owner->setCallback(std::in_place_type<Event>, name,
                               std::forward<Callback>(callback));
            return std::make_unique<CallbackHolder<Owner, Event>>(owner, name);
        }
        return {};
    }
}
