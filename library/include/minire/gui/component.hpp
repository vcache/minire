#pragma once

#include <minire/errors.hpp>
#include <minire/gui/area.hpp>
#include <minire/gui/arranger.hpp>
#include <minire/gui/callbacks.hpp>
#include <minire/gui/content-view.hpp>
#include <minire/gui/layout.hpp>
#include <minire/gui/property.hpp>
#include <minire/gui/theme.hpp>
#include <minire/utils/rect.hpp>

#include <any>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace minire { class GuiController; }

namespace minire::gui
{
    class OverlayController;

    /**
     * A note on consistency: properties and other states of a Component
     * might be observed as unconsistent at particular moments of time.
     * These state will become consistent after the proximate revalidation
     * call. In other words, a Component follows "eventual consistency"
     * principle.
     *
     * For example, in a ListView component, the contents() value can be cleared
     * while selected() retains a reference to a non-existing index. After the
     * next revalidation cycle the value of selected() will be cleared as well and
     * correponding events will be issued.
     * */
    class Component
        : public CommonCallbacks<Component>
        , public ContentInvalidator
        , public std::enable_shared_from_this<Component>
    {
    public:
        using Sptr = std::shared_ptr<Component>;
        using Wptr = std::weak_ptr<Component>;
        using Children = std::unordered_map<std::string, Sptr>;

        Component(std::string const & id,
                  Theme const & theme,
                  OverlayController & overlayController);
        ~Component();

        using CommonCallbacks::handle;

    public:
        // Generic properties
        std::string const & id() const { return _id; }

        Property<bool> const & visible() const { return _visible; }
        Property<bool> & visible() { return _visible; }

        Property<bool> const & isDraggable() const { return _isDraggable; }
        Property<bool> & isDraggable() { return _isDraggable; }

        Property<Arranger> const & horizontal() const { return _horizontal; }
        Property<Arranger> & horizontal() { return _horizontal; }

        Property<Arranger> const & vertical() const { return _vertical; }
        Property<Arranger> & vertical() { return _vertical; }

        Property<utils::Rect> const & padding() const { return _padding; }
        Property<utils::Rect> & padding() { return _padding; }

        Property<size_t> const & zOrder() const { return _zOrder; }
        Property<size_t> & zOrder() { return _zOrder; }

        Property<Layout::Sptr> const & layout() const { return _layout; }
        Property<Layout::Sptr> & layout() { return _layout; }

        // User's opaque data
        template<typename T>
        void setUserData(T && userData)
        {
            _userData = std::forward<T>(userData);
        }

        template<typename T, class... Args>
        void emplaceUserData(Args && ... args)
        {
            _userData.emplace<T>(std::forward<Args>(args)...);
        }

        template<typename T, typename U, class... Args>
        void emplaceUserData(std::initializer_list<U> il,
                             Args && ... args)
        {
            _userData.emplace<T>(il, std::forward<Args>(args)...);
        }

        std::any const & userData() const { return _userData; }
        std::any & userData() { return _userData; }

        // Component tree navigation (read-only)
        Children const & children() const { return _children; }
        Sptr parent() const { return _parent.lock(); }

        // State flags
        bool hasFocus() const { return _hasFocus; }
        bool isHovered() const { return _isHovered; }
        bool isDragging() const { return _isHovered; }

    public:
        bool contains(std::string const & childId) const { return _children.contains(childId); }

        template<typename T>
        std::shared_ptr<T> find(std::string const & childId) const
        {
            auto it = _children.find(childId);
            return it != _children.cend() ? std::dynamic_pointer_cast<T>(it->second)
                                          : std::shared_ptr<T>();
        }

        template<typename T>
        std::shared_ptr<T> find(std::string const & childId)
        {
            return std::const_pointer_cast<T>(
                const_cast<Component const *>(this)->find<T>(childId));
        }

        template<typename T>
        T const & at(std::string const & childId) const
        {
            auto it = _children.find(childId);
            MINIRE_INVARIANT(it != _children.cend(),
                             "no such component: \"{}\" in a container \"{}\"",
                             childId, _id);
            assert(it->second);
            return dynamic_cast<T &>(*it->second);
        }

        template<typename T>
        T & at(std::string const & childId)
        {
            return const_cast<T &>(const_cast<Component const *>(this)->at<T>(childId));
        }

        virtual std::optional<std::pair<float, float>> measureContent() const
        {
            return std::nullopt;
        }

    public:
        // Various mutators
        void setParent(Sptr const &);

        template<typename T, typename ... Args>
        std::shared_ptr<T> emplace(std::string const childId,
                                   Args &&... args)
        {
            std::shared_ptr<T> child = std::make_shared<T>(
                childId, _theme, _overlayController,
                std::forward<Args>(args)...);
            child->setParent(shared_from_this());
            return child;
        }

        void invalidate();

        void invalidateContent() override;

        void erase(std::string const &);

        void clear();

    protected:
        virtual void initialize() {}

        virtual size_t revalidateContent(size_t zOffset,
                                         bool const /*effectiveVisible*/,
                                         Area const & /*clientArea*/)
        {
            return zOffset;
        }

        Theme const & theme() const { return _theme; }

        OverlayController & overlayController() { return _overlayController; }

    private:
        size_t revalidate(size_t zOffset,
                          bool effectiveVisible,
                          Area const clientArea);

        // NOTE: Since this method can return a pointer to "this" w/o a const-qualifier,
        //       this method implicitly works as a "const cast" (TODO: it is not good probably?).
        Sptr findUnderCursor(float x, float y) const;

    private:
        class Impl;

        std::string const         _id;
        Theme const             & _theme;
        OverlayController       & _overlayController;

        Property<bool>            _visible;
        Property<bool>            _isDraggable;
        Property<Arranger>        _horizontal;
        Property<Arranger>        _vertical;
        Property<utils::Rect>     _padding;
        Property<size_t>          _zOrder;
        Property<Layout::Sptr>    _layout;

        std::any                  _userData;

        Wptr                      _parent;
        Children                  _children;
        std::unique_ptr<Impl>     _impl;

        bool                      _invalidated;
        bool                      _contentInvalidated;

        bool                      _hasFocus;
        bool                      _isHovered;
        bool                      _isDragging;

        friend class minire::GuiController;
    };
}
