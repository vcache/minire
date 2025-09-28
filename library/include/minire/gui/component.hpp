#pragma once

#include <minire/content/id.hpp>
#include <minire/events/application.hpp>
#include <minire/events/controller.hpp>
#include <minire/gui/area.hpp>
#include <minire/gui/arranger.hpp>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_set>
#include <utility>
#include <vector>

namespace minire { class GuiController; }
namespace minire::content { class Lease; }
namespace minire::gui::components { class Container; }
namespace minire::text { class FormattedString; }

namespace minire::gui
{
    class Component
        : public std::enable_shared_from_this<Component>
    {
    public:
        Component(GuiController & controller,
                  std::string const & id,
                  std::shared_ptr<components::Container> const & parent)
            : _controller(controller)
            , _id(id)
            , _parent(parent)
        {
            invalidateZOrder();
        }

        virtual ~Component();

        using Sptr = std::shared_ptr<Component>;
        using Wptr = std::weak_ptr<Component>;
        using ZOrderUpdates = std::vector<std::pair<std::string, size_t>>;

    public:
        std::string const & id() const { return _id; }

        bool visible() const { return _visible; }
        void setVisible(bool const);

        void setArrangers(Arrangers arrangers);
        Arrangers const & arrangers() const { return _arrangers; }

        // NOTE: this is local z (relative to the container)
        size_t zOrder() const { return _zOrder; }
        void setZOrder(size_t const);

        std::shared_ptr<components::Container> parent() const { return _parent.lock(); }

        void setClientArea(Area clientArea);

        /**
         * Should be called by a client after any change.
         * */
        size_t revalidateZOrder(size_t offset, ZOrderUpdates & labels,
                                ZOrderUpdates & sprites);

        template<typename T>
        bool handle(T const & event)
        {
            if (_visible && isSubscribed<T>())
            {
                onEvent(event);
                return true;
            }
            return false;
        }

    protected:
        void invalidateZOrder();

    protected:
        /**
         * Will be called only when visible() value is actually changed.
         * */
        virtual void onVisibleChanged() = 0;

        /**
         * Will be called only when contentArea() value is actually changed.
         * */
        virtual void onContentAreaChanged() = 0;

        /**
         * A component should update z-order of its sprites and labels,
         * and return a new value of an offset. For example, if a component
         * has 2 images, it must return (offset + 2).
         * */
        virtual size_t onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                                       ZOrderUpdates & sprites) = 0;

        virtual void onChildSubscriptionChanged() {}

        // TODO: why not glm::vec2 ?
        virtual std::optional<std::pair<float, float>> measureContent() const;

        Area const & contentArea() const { return _contentArea; }

        void rearrange();

        void enqueueRaw(events::Controller &&);

        template<typename EventType,
                 typename... Args>
        void enqueue(Args && ... args)
        {
            enqueueRaw(EventType(std::forward<Args>(args)...));
        }

        void focus();
        void unfocus();

    protected:
        template<typename T>
        constexpr static auto const & cleanTypeId()
        {
            return typeid(std::decay_t<T>);
        }

        template<typename T>
        void subscribe()
        {
            _eventsFilter.emplace(cleanTypeId<T>());
            notifyParentOnSubscription();
        }

        template<typename T>
        void unsubscribe()
        {
            _eventsFilter.erase(cleanTypeId<T>());
            notifyParentOnSubscription();
        }

        template<typename T>
        bool isSubscribed() const { return _eventsFilter.contains(cleanTypeId<T>()); }

        virtual void onEvent(events::application::OnMouseWheel const &) {}
        virtual void onEvent(events::application::OnMouseMove const &) {}
        virtual void onEvent(events::application::OnMouseDown const &) {}
        virtual void onEvent(events::application::OnMouseUp const &) {}
        virtual void onEvent(events::application::OnKeyUp const &) {}
        virtual void onEvent(events::application::OnKeyDown const &) {}
        virtual void onEvent(events::application::OnTextInput const &) {}

        // TODO: add some high-level events, such as onClick, onMouseEnter, onMouseLeave
        //       onFocus, onUnfocue

    protected: // Shortcuts to a GuiController
        std::unique_ptr<content::Lease> borrow(content::Id const &);

        glm::vec2 measure(text::FormattedString const & text,
                          content::Id const & id) const;

    private:
        void notifyParentOnSubscription();

    private:
        using EventsFilter = std::unordered_set<std::type_index>; // TODO: why not std::bitset?
        using ParentWptr = std::weak_ptr<components::Container>;
        using ZOrderBoundaries = std::pair<size_t, size_t>;

        GuiController   & _controller;
        std::string const _id;
        ParentWptr        _parent;
        Arrangers         _arrangers;
        Area              _clientArea;
        Area              _contentArea;
        EventsFilter      _eventsFilter;
        size_t            _zOrder = 0;
        ZOrderBoundaries  _zOrderBoundaries{0, 0}; // the first and last offsets, like (begin, end)
        bool              _zOrderInvalidated = true;
        bool              _visible = true;

        friend class components::Container;
    };
}