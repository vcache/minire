#include <minire/gui/component.hpp>

#include <minire/errors.hpp>
#include <minire/gui/overlay-controller.hpp>

#include <algorithm>
#include <cassert>
#include <list>

namespace minire::gui
{
    static constexpr size_t kExpectedChildren = 25;
    static constexpr size_t kZOrderIndent = 1'000'000;

    namespace
    {
        class RaiiFlag
        {
            RaiiFlag(RaiiFlag const &) = delete;
            RaiiFlag(RaiiFlag &&) = delete;
            RaiiFlag & operator=(RaiiFlag const &) = delete;
            RaiiFlag & operator=(RaiiFlag &&) = delete;

        public:
            explicit RaiiFlag(bool & target)
                : _target(target)
            {
                _target = true;
            }

            ~RaiiFlag()
            {
                _target = false;
            }

        private:
            bool & _target;
        };
    }

    class Component::Impl
    {
    public:
        struct ComponentZCompare
        {
            bool operator()(Sptr const & lhs, Sptr const & rhs) const
            {
                assert(lhs);
                assert(rhs);

                if (lhs->_zOrder.get() < rhs->_zOrder.get()) return true;
                if (lhs->_zOrder.get() > rhs->_zOrder.get()) return false;

                return lhs.get() < rhs.get();
            };
        };

        using ZOrderStore = std::list<Component::Sptr>;
        using ZOrderBoundaries = std::pair<size_t, size_t>;

    public:
        Area             _clientArea;
        Area             _contentArea;
        Area             _contentClippingWindow;
        ZOrderStore      _zOrderStore; // in an ascending zOrder
        ZOrderBoundaries _zOrderBoundaries{0, 0};
        size_t           _zOrder = 0;
        bool             _zOrderInvalidated = true;
        bool             _zOrderLocked = false;
        bool             _effectiveVisible = true;
        bool             _visible = true;
        bool             _initialized = false;

    public:
        void eraseZOrderStore(Component::Sptr const & component)
        {
            MINIRE_INVARIANT(!_zOrderLocked, "attempt to modify zOrder while it is locked");
            assert(component);
            auto it = std::ranges::find(_zOrderStore, component);
            MINIRE_INVARIANT(it != _zOrderStore.end(), "_zOrderStore don't contain \"{}\"",
                             component->_id);

            _zOrderStore.erase(it);
            _zOrderInvalidated = true;
        }

        void insertZOrderStore(Component::Sptr const & component)
        {
            MINIRE_INVARIANT(!_zOrderLocked, "attempt to modify zOrder while it is locked");
            assert(component);
            assert(_zOrderStore.end() == std::ranges::find(_zOrderStore, component)); // test dups

            _zOrderStore.emplace_back(component);
            sortZOrderStore();

            _zOrderInvalidated = true;
        }

        void sortZOrderStore()
        {
            MINIRE_INVARIANT(!_zOrderLocked, "attempt to modify zOrder while it is locked");
            _zOrderStore.sort(ComponentZCompare{});
        }

        size_t getHighestZOrder() const
        {
            assert(_zOrderStore.empty() || _zOrderStore.back());
            return !_zOrderStore.empty() ? _zOrderStore.back()->_zOrder.get() : 0;
        }
    };

    Component::Component(std::string const & id,
                         Theme const & theme,
                         Theme::Style const & style,
                         OverlayController & overlayController)
        : _id(id)
        , _theme(theme)
        , _style(style)
        , _overlayController(overlayController)
        , _visible(*this, true)
        , _isDraggable(*this, false)
        , _horizontal(*this)
        , _vertical(*this)
        , _padding(*this)
        , _zOrder(*this)
        , _layout(*this, std::make_shared<LinearLayout>())
        , _systemCursor(models::SystemCursor::kArrow)
        , _impl(std::make_unique<Impl>())
        , _invalidated(false)
        , _contentInvalidated(false)
        , _hasFocus(false)
        , _isHovered(false)
        , _isDragging(false)
        , _acceptFocus(false)
        , _eventTransparent(false)
    {
        _children.reserve(kExpectedChildren);
        invalidate();
    }

    Component::~Component() = default;

    void Component::setParent(Sptr const & newParent)
    {
        // check preconditions
        if (newParent && newParent->_children.contains(_id))
        {
            MINIRE_THROW("Component \"{}\" already contains \"{}\"",
                         newParent->_id, _id);
        }

        auto sharedThis = shared_from_this();

        // remove self from previous parent (if any)
        if (auto oldParent = parent(); oldParent)
        {
            size_t const removed = oldParent->_children.erase(_id);
            MINIRE_INVARIANT(removed == 1, "failed to remove \"{}\" from \"{}\"",
                             _id, oldParent->_id);

            // update zOrderStore
            assert(oldParent->_impl);
            oldParent->_impl->eraseZOrderStore(sharedThis);

            // update Layout
            Layout::Sptr & oldParentLayout = *(oldParent->_layout);
            assert(oldParentLayout);
            oldParentLayout->onErase(*this);

            oldParent->invalidate();
        }

        // insert into a new parent (id any)
        if (newParent)
        {
            // insert self into a _children store
            auto [_, inserted] = newParent->_children.emplace(_id, sharedThis);
            MINIRE_INVARIANT(inserted, "failed to insert \"{}\" into \"{}\"",
                             _id, newParent->_id);

            // recalc self's zOrder
            assert(newParent->_impl);
            _zOrder = newParent->_impl->getHighestZOrder() + 1;
            newParent->_impl->insertZOrderStore(sharedThis);

            // update layout
            Layout::Sptr & newParentLayout = *(newParent->_layout);
            assert(newParentLayout);
            newParentLayout->onInsert(*this);

            // invalidate parent's state
            newParent->invalidate();
        }

        _parent = newParent;
        invalidate();
    }

    void Component::invalidate()
    {
        // NOTE: *this Component _might_ be not yet re-validated,
        //       so that the _invalidated can be already set.
        //       But there is a guarantee, that every parent Component
        //       has been revalidated.
        _invalidated = true;

        for (auto p = parent(); p && !p->_invalidated; p = p->parent())
        {
            p->_invalidated = true;
        }

#       ifndef NDEBUG
        // Slightly overkill, but that part was problematic,
        // so I became paranoid.
        bool fullChainInvalidated = _invalidated;
        for(auto p = parent(); fullChainInvalidated && p; p = p->parent())
            fullChainInvalidated &= p->_invalidated;
        assert(fullChainInvalidated);
#       endif
    }

    void Component::invalidateContent()
    {
        _contentInvalidated = true;
        invalidate();
    }

    void Component::erase(std::string const & childId)
    {
        if (auto it = _children.find(childId);
            it != _children.end())
        {
            Sptr child = it->second;
            assert(child);
            child->setParent(nullptr);
        }
    }

    void Component::clear()
    {
        _impl = std::make_unique<Impl>();
        assert(_layout.get());
        _layout.get()->onClear();
        _children.clear();
        invalidate();
    }

    void Component::setAcceptFocus(bool acceptFocus)
    {
        _acceptFocus = acceptFocus;
        // TODO: drop focus (if any) when it become false
    }

    void Component::setEventTransparent(bool eventTransparent)
    {
        _eventTransparent = eventTransparent;
    }

    void Component::setSystemCursor(models::SystemCursor const systemCursor)
    {
        if (systemCursor != _systemCursor)
        {
            _systemCursor = systemCursor;
            if (hasFocus())
            {
                overlayController().setSystemCursor(systemCursor);
            }
        }
    }

    // TODO: add metric the amount of revalidations to detect loops and
    //       too intensive/pointless revalidations
    size_t Component::revalidate(size_t zOffset,
                                 bool effectiveVisible,
                                 Area const & clientArea,
                                 Area const & clippingWindow)
    {
        // drop this flag at the beffining, so that descendants and children
        // may re-set it immediately. This will increase flexibility (however,
        // it creates the possibility of a hot loop).
        _invalidated = false;

        assert(zOffset != 0); // should start from 1
        assert(_impl);

        if (!_impl->_initialized)
        {
            initialize();
            _impl->_initialized = true;
        }

        bool revalidateChildren = false;

        // revalidate visibillity
        if (_visible.isInvalidated() &&
            _impl->_visible != _visible.get())
        {
            _impl->_visible = _visible.get();
        }
        _visible.revalidate();

        effectiveVisible &= _impl->_visible;
        bool const effectiveVisibleChanged = _impl->_effectiveVisible != effectiveVisible;
        bool const effectiveVisibleEnabled = !_impl->_effectiveVisible && effectiveVisible;
        _impl->_effectiveVisible = effectiveVisible;

        // revalidate Component's arrangement
        if (effectiveVisibleEnabled ||
            clientArea != _impl->_clientArea ||
            _horizontal.isInvalidated() ||
            _vertical.isInvalidated() ||
            _padding.isInvalidated() ||
            _contentInvalidated)
        {
            if (_impl->_visible)
            {
                std::optional<glm::vec2> contentSize = measureContent();

                auto [left, width] = _horizontal.get()(clientArea._left,
                                                       clientArea._width,
                                                       contentSize ? std::optional<float>(contentSize->x)
                                                                   : std::nullopt);

                auto [top, height] = _vertical.get()(clientArea._top,
                                                     clientArea._height,
                                                     contentSize ? std::optional<float>(contentSize->y)
                                                                 : std::nullopt);

                Area contentArea
                {
                    ._left = left,
                    ._top = top,
                    ._width = width,
                    ._height = height,
                };
                if (contentArea != _impl->_contentArea)
                {
                    _impl->_contentArea = contentArea;
                    revalidateChildren = true;
                }
            }
            _impl->_clientArea = clientArea;
        }
        _horizontal.revalidate();
        _vertical.revalidate();
        _padding.revalidate();
        _contentInvalidated = false;

        // maybe change zOrder
        if (_zOrder.isInvalidated() &&
            _impl->_zOrder != _zOrder.get())
        {
            _impl->_zOrder = _zOrder.get();
            _impl->_zOrderInvalidated = true;
        }
        _zOrder.revalidate();

        // revalidate z-order
        if (_impl->_zOrderInvalidated ||
            zOffset > _impl->_zOrderBoundaries.first)
        {
            _impl->_zOrderBoundaries.first = zOffset;
            _impl->_zOrderInvalidated = false;
            revalidateChildren = true;
        }

        // revalidate derivated Component
        Area const & clientClippingWindow = intersection(clippingWindow, _impl->_clientArea);
        Area const & contentClippingWindow = intersection(clientClippingWindow, _impl->_contentArea);
        zOffset = revalidateContent(zOffset, effectiveVisible, _impl->_contentArea, contentClippingWindow);
        if (contentClippingWindow != _impl->_contentClippingWindow)
        {
            _impl->_contentClippingWindow = contentClippingWindow;
            revalidateChildren = true;
        }

        // revalidate Layout
        revalidateChildren |= _layout.isInvalidated();
        _layout.revalidate();

        // revalidate children
        assert(_impl->_zOrderStore.size() == _children.size());

        utils::Rect const padding = _padding.get(); //  TODO: maybe padding should affect _contentArea instead?
                                                    //        (or just decrease clientArea)

        Area const childrenClientArea = Area
        {
            ._left = _impl->_contentArea._left + padding._left,
            ._top = _impl->_contentArea._top + padding._top,
            ._width = std::max(.0f, _impl->_contentArea._width - (padding._left + padding._right)),
            ._height = std::max(.0f, _impl->_contentArea._height - (padding._top + padding._bottom)),
        };

        Area const & childrenClientClippingWindow = intersection(contentClippingWindow, childrenClientArea);

        {
            // prepare z-order store
            _impl->sortZOrderStore(); // TODO: don't resort every time (maybe use invalidation flags instead bool)
            RaiiFlag zOrderStoreLock(_impl->_zOrderLocked);

            // fetch the Layout
            Layout::Sptr const & layout = _layout.get();
            assert(layout);

            // prepare target for Layout
            Layout::Targets targets;
            targets.reserve(_impl->_zOrderStore.size());
            for(Sptr const & child : _impl->_zOrderStore)
            {
                assert(child);
                targets.emplace_back(Layout::Target
                {
                    ._component = *child,
                });
            }
            Layout::Areas const & childrenContentAreas = layout->evaluate(childrenClientArea, targets);
            assert(childrenContentAreas.size() == _impl->_zOrderStore.size());

            // revalidare the children
            size_t index = 0;
            for(Sptr const & child : _impl->_zOrderStore)
            {
                assert(child);
                assert(child->_impl);

                size_t newOffset = child->_impl->_zOrderBoundaries.second;
                bool const visibilityTest = effectiveVisibleChanged ||
                                            child->_visible.isInvalidated() ||
                                            child->_visible.get() ||
                                            child->_visible.get() != child->_impl->_visible;

                if (child->_invalidated ||
                    (visibilityTest && (revalidateChildren || effectiveVisibleChanged)))
                {
                    assert(index < childrenContentAreas.size());
                    Area const & childArea = childrenContentAreas[index];
                    Area const & childClippingWindow = intersection(childrenClientClippingWindow, childArea);
                    newOffset = child->revalidate(zOffset, effectiveVisible, childArea, childClippingWindow);
                    assert(newOffset == child->_impl->_zOrderBoundaries.second);
                }

                zOffset = std::max(newOffset, zOffset + kZOrderIndent);
                index++;
            }
        }

        _impl->_zOrderBoundaries.second = zOffset;

        // finish
        return _impl->_zOrderBoundaries.second;
    }

    Component::Sptr Component::findUnderCursor(float x, float y) const
    {
        assert(_impl);

        if (!_visible.get() ||
            _eventTransparent ||
            !_impl->_contentArea.contains(x, y))
        {
            return Sptr();
        }

        Sptr result;

        for(auto it = _impl->_zOrderStore.rbegin();
            it != _impl->_zOrderStore.rend() && !result;
            ++it)
        {
            assert(*it);

            if ((*it)->eventTransparent())
                continue;

            result = (*it)->findUnderCursor(x, y);
        }

        return result ? result : std::const_pointer_cast<Component>(shared_from_this());
    }
}
