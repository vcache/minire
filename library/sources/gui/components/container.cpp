#include <minire/gui/components/container.hpp>

#include <type_traits>

namespace minire::gui::components
{
    static constexpr size_t kZOrderIndent = 1'000'000;

    void Container::clear()
    {
        _children.clear();
        _zOrderStore.clear();
        onChildSubscriptionChanged();
    }

    void Container::erase(std::string const & id)
    {
        if (auto it = _children.find(id);
            it != _children.end())
        {
            Component::Sptr const & child = it->second;
            _zOrderStore.erase(child);
            _children.erase(it);
            onChildSubscriptionChanged();
        }
    }

    void Container::onVisibleChanged()
    {
        for(auto const & [_, child] : _children)
        {
            assert(child);
            child->setVisible(_visible);
        }
        onChildSubscriptionChanged();
    }

    void Container::updateChildrenContentArea(Component::Sptr const & child)
    {
        assert(child);
        assert(_layout);
        Area const & childArea = _layout->evaluate(_clientArea, *child);
        child->setClientArea(childArea);
    }

    void Container::onContentAreaChanged()
    {
        for(auto const & [_, child] : _children)
        {
            updateChildrenContentArea(child);
        }
    }

    size_t Container::onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                                      ZOrderUpdates & sprites)
    {
        for(Component::Sptr const & comp : _zOrderStore)
        {
            size_t newOffset = comp->revalidateZOrder(offset, labels, sprites);
            offset = std::max(newOffset, offset + kZOrderIndent);
        }
        return offset;
    }

    void Container::onChildSubscriptionChanged()
    {
        EventsFilter newEventsFilter;
        newEventsFilter.reserve(_eventsFilter.size());
        for(auto const & [_, child] : _children)
        {
            assert(child);
            newEventsFilter.insert(child->_eventsFilter.begin(),
                                   child->_eventsFilter.end());
        }
        if (newEventsFilter == _eventsFilter)
            return;

        _eventsFilter = std::move(newEventsFilter);
        notifyParentOnSubscription();
    }

    void Container::onEvent(events::application::OnMouseWheel const & e)
    {
        broadcast(e);
    }
    
    void Container::onEvent(events::application::OnMouseMove const & e)
    {
        sendUnderCursor(e);
    }
    
    void Container::onEvent(events::application::OnMouseDown const & e)
    {
        sendUnderCursor(e);
    }
    
    void Container::onEvent(events::application::OnMouseUp const & e)
    {
        sendUnderCursor(e);
    }
    
    void Container::onEvent(events::application::OnKeyUp const & e)
    {
        broadcast(e);
    }
    
    void Container::onEvent(events::application::OnKeyDown const & e)
    {
        broadcast(e);
    }
    
    void Container::onEvent(events::application::OnTextInput const & e)
    {
        broadcast(e);
    }

    template<typename T>
    void Container::broadcast(T const & e)
    {
        for(auto & [_, child] : _children)
        {
            assert(child);
            child->handle(e);
        }
    }

    template<typename T>
    void Container::sendUnderCursor(T const & e)
    {
        float x = 0;
        float y = 0;
        if constexpr(std::is_same_v<T, events::application::OnMouseMove>)
        {
            x = static_cast<float>(e._absX);
            y = static_cast<float>(e._absY);
        }
        else
        {
            x = static_cast<float>(e._x);
            y = static_cast<float>(e._y);
        }

        for(auto it = _zOrderStore.rbegin(); it != _zOrderStore.rend(); ++it)
        {
            auto child = *it;
            assert(child);
            if (child->contentArea().contains(x, y) && child->handle(e))
            {
                break;
            }
        }
    }
}