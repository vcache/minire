#pragma once

#include <minire/content/id.hpp>
#include <minire/events/application.hpp>
#include <minire/events/controller.hpp>
#include <minire/gui/area.hpp>
#include <minire/gui/arranger.hpp>
#include <minire/utils/rect.hpp>

#include <memory>
#include <optional>
#include <string>
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
        T & as() { return dynamic_cast<T &>(*this); }

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

        // TODO: why not glm::vec2 ?
        virtual std::optional<std::pair<float, float>> measureContent() const;

        Area const & contentArea() const { return _contentArea; }

        void rearrange(bool force = false);

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
        virtual void onEvent(events::application::OnMouseWheel const &) {}
        virtual void onEvent(events::application::OnMouseMove const &) {}
        virtual void onEvent(events::application::OnMouseDown const &) {}
        virtual void onEvent(events::application::OnMouseUp const &) {}
        virtual void onEvent(events::application::OnKeyUp const &) {}
        virtual void onEvent(events::application::OnKeyDown const &) {}
        virtual void onEvent(events::application::OnTextInput const &) {}

        virtual void onMouseEnter(bool /*isClickReturn*/) {}
        virtual void onMouseLeave() {}  // will be delivered even to a non-visible one

        virtual void onFocus() {}
        virtual void onUnfocus() {}     // will be delivered even to a non-visible one

        virtual void onClick() {}

        bool isHovered() const { return _isHovered; }

    protected: // Shortcuts to a GuiController
        std::unique_ptr<content::Lease> borrow(content::Id const &) const;

        glm::vec2 measure(text::FormattedString const & text,
                          content::Id const & id) const;

        std::pair<glm::vec2, bool> measure(utils::Patch const &,
                                           content::Id const & texture) const;

    private:
        using ParentWptr = std::weak_ptr<components::Container>;
        using ZOrderBoundaries = std::pair<size_t, size_t>;

        GuiController   & _controller;
        std::string const _id;
        ParentWptr        _parent;
        Arrangers         _arrangers;
        Area              _clientArea;
        Area              _contentArea;
        size_t            _zOrder = 0;
        ZOrderBoundaries  _zOrderBoundaries{0, 0}; // the first and last offsets, like (begin, end)
        bool              _zOrderInvalidated = true;
        bool              _visible = true;
        bool              _isHovered = false;

        friend class components::Container;
        friend class ::minire::GuiController;
        friend class Layout; // for rearrange()
    };
}