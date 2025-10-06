#include <minire/gui/components/container.hpp>

#include <minire/gui-controller.hpp>
#include <minire/logging.hpp>

#include <cassert>

namespace minire::gui::components
{
    static constexpr size_t kZOrderIndent = 1'000'000;

    void Container::emplace(std::shared_ptr<Component> const & child)
    {
        if (!child) return;

        MINIRE_INVARIANT(&_controller == &child->_controller,
                         "cannot transfer Component with a different Controller");

        if (auto previousParent = child->_parent.lock();
            previousParent)
        {
            if (previousParent.get() == this)
            {
                MINIRE_WARNING("Component \"{}\" is already stored in \"{}\"",
                               child->id(), id());
                return;
            }

            previousParent->erase(child->id());
        }

        MINIRE_INVARIANT(!_children.contains(child->id()),
                         "Container \"{}\" already has child: \"{}\"",
                         id(), child->id());

        child->_parent = sharedThis();
        emplaceImpl(child);
    }

    void Container::emplaceImpl(std::shared_ptr<Component> const & child)
    {
        assert(child);

        auto [_, inserted] = _children.emplace(child->id(), child);
        MINIRE_INVARIANT(inserted, "failed to insert parent component: \"{}\" into \"{}\"",
                         child->id(), id());
        if (!_zOrderStore.empty())
        {
            Component::Sptr const & last = *_zOrderStore.rbegin();
            assert(last);
            child->_zOrder = last->_zOrder + 1;
        }
        _zOrderStore.emplace(child);
        updateChildrenContentArea(child);
        child->invalidateZOrder();
    }

    void Container::clear()
    {
        _children.clear();
        _zOrderStore.clear();
        _layout->onClear();
    }

    void Container::erase(std::string const & id)
    {
        if (auto it = _children.find(id);
            it != _children.end())
        {
            Component::Sptr const & child = it->second;
            assert(child->_parent.lock().get() == this);

            // clean up tha child
            _layout->onErase(*child);
            _zOrderStore.erase(child);
            child->_parent.reset();

            // erase the child
            _children.erase(it);

            rearrange();
        }
    }

    void Container::onVisibleChanged()
    {
        for(auto const & [_, child] : _children)
        {
            assert(child);
            child->setVisible(_visible);
        }
    }

    void Container::updateChildrenContentArea(Component::Sptr const & child)
    {
        assert(child);
        assert(_layout);
        if (child->visible())
        {
            Area const & childArea = _layout->evaluate(_contentArea, *child);
            child->setClientArea(childArea);
        }
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

    Component::Sptr Container::findUnderCursor(float x, float y)
    {
        if (!_visible || !_contentArea.contains(x, y))
            return {};

        for(auto it = _zOrderStore.rbegin(); it != _zOrderStore.rend(); ++it)
        {
            auto child = *it;
            if (child && child->visible() && child->contentArea().contains(x, y))
            {
                if (auto asContainer = std::dynamic_pointer_cast<Container>(child);
                    asContainer)
                {
                    return asContainer->findUnderCursor(x, y);
                }
                else
                {
                    return child;
                }
            }
        }

        return shared_from_this();
    }
}
