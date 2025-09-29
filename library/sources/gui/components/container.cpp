#include <minire/gui/components/container.hpp>

#include <minire/gui-controller.hpp>

namespace minire::gui::components
{
    static constexpr size_t kZOrderIndent = 1'000'000;

    void Container::clear()
    {
        _children.clear();
        _zOrderStore.clear();
    }

    void Container::erase(std::string const & id)
    {
        if (auto it = _children.find(id);
            it != _children.end())
        {
            Component::Sptr const & child = it->second;
            _zOrderStore.erase(child);
            _children.erase(it);
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
        Area const & childArea = _layout->evaluate(_contentArea, *child);
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
