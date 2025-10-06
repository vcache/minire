#pragma once

#include <minire/content/id.hpp>
#include <minire/events/application.hpp>
#include <minire/events/controller.hpp>
#include <minire/gui/area.hpp>
#include <minire/gui/arranger.hpp>
#include <minire/models/input-handler.hpp>
#include <minire/utils/rect.hpp>

#include <any>
#include <functional>
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
        , public minire::models::InputHandler
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

    public:
        bool isDragable() const { return _isDragable; }
        void setIsDragable(bool v) { _isDragable = v; }

        bool isDragging() const { return _isDragging; }

        using DragBeginCallback =
            std::function<void(Component &, events::application::OnMouseDown const &)>;

        using DragMoveCallback =
            std::function<void(Component &, events::application::OnMouseMove const &)>;

        using DragEndCallback =
            std::function<void(Component &, std::optional<events::application::OnMouseUp> const &)>;

        template<typename Callback>
        void setDragBeginCallback(Callback c) { _dragBeginCallback = c; }

        template<typename Callback>
        void setDragMoveCallback(Callback c) { _dragMoveCallback = c; }

        template<typename Callback>
        void setDragEndCallback(Callback c) { _dragEndCallback = c; }

        virtual void onDragBegin(events::application::OnMouseDown const &);
        virtual void onDragMove(events::application::OnMouseMove const &);
        // NOTE: when e == std::nullopt, dragging is cancelled
        virtual void onDragEnd(std::optional<events::application::OnMouseUp> const &);

    public:
        std::any const & userData() const { return _userData; }

        void setUserData(std::any v) { _userData = std::move(v); }

    public:
        // Relative to a parent
        Area const & contentArea() const { return _contentArea; }

        // Abosulute values
        Area const & clientArea() const { return _clientArea; }

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

        gui::components::Container & guiPush(std::string,
            minire::models::InputHandler::Wptr = {});
        std::string const & guiTopTag() const;
        void guiPop();

    private:
        using ParentWptr = std::weak_ptr<components::Container>;
        using ZOrderBoundaries = std::pair<size_t, size_t>;

        GuiController   & _controller;
        std::string const _id;
        std::any          _userData;
        ParentWptr        _parent;
        DragBeginCallback _dragBeginCallback;
        DragMoveCallback  _dragMoveCallback;
        DragEndCallback   _dragEndCallback;
        Arrangers         _arrangers;
        Area              _clientArea;
        Area              _contentArea;
        size_t            _zOrder = 0;
        ZOrderBoundaries  _zOrderBoundaries{0, 0}; // the first and last offsets, like (begin, end)
        bool              _zOrderInvalidated = true;
        bool              _visible = true;
        bool              _isHovered = false;
        bool              _isDragable = false;
        bool              _isDragging = false;

        friend class components::Container;
        friend class ::minire::GuiController;
        friend class Layout; // for rearrange()
    };
}