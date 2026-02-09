#include <minire/gui/layouts/flow.hpp>

#include <minire/errors.hpp>
#include <minire/gui/component.hpp>
#include <minire/logging.hpp>

#include <utils/overloaded.hpp>

#include <cassert>

namespace minire::gui::layouts
{
    Flow::Flow(bool horizontal,
               std::list<Element> const & elements)
        : _horizontal(horizontal)
    {
        _index.reserve(elements.size());
        for(Element const & element : elements)
        {
            _heap.emplace_back(Slot{element});
            storeIndex(element, std::prev(_heap.end()));
        }
    }

    Layout::Areas Flow::evaluate(Area const & clientArea,
                                 Layout::Targets const & targets) const
    {
        // measure flow elements
        for(Layout::Target const & target : targets)
        {
            gui::Component const & component = target._component;
            std::optional<std::pair<float, float>> const & componentSize = component.measureContent();
            MINIRE_INVARIANT(componentSize, "Flow layout requires measurable children, but \"{}\" is not",
                             component.id());

            if (auto it = _index.find(component.id());
                it != _index.cend())
            {
                assert(it->second != _heap.end());
                Slot & slot = *(it->second);
                slot._offset = 0;
                slot._width = componentSize->first;
                slot._height = componentSize->second;
            }
            else
            {
                MINIRE_WARNING("unknown Flow layout target: {}", component.id());
            }
        }

        // calculate offsets
        float offset = 0;
        for(Slot & slot : _heap)
        {
            slot._offset = offset;
            offset += std::visit(
                utils::Overloaded
                {
                    [&slot, this](Component const &) -> float
                    {
                        return _horizontal ? slot._width : slot._height;
                    },
                    [](Spacing const & spacing) -> float { return spacing._value; },
                }, slot._element);
        }

        // build the result
        Layout::Areas result;
        result.reserve(targets.size());
        for(Layout::Target const & target : targets)
        {
            if (auto it = _index.find(target._component.id());
                it != _index.cend())
            {
                assert(it->second != _heap.end());
                Slot const & slot = *(it->second);
                result.emplace_back(Area
                {
                    ._left = clientArea._left + (_horizontal ? slot._offset : 0),
                    ._top = clientArea._top + (_horizontal ? 0 : slot._offset),
                    ._width = _horizontal ? slot._width : clientArea._width,
                    ._height = _horizontal ? clientArea._height : slot._height,
                });
            }
            else
            {
                result.emplace_back(Area());
            }
        }

        // finish
        return result;
    }

    void Flow::onErase(gui::Component const & component)
    {
        erase(component.id());
    }

    void Flow::onClear()
    {
        _heap.clear();
        _index.clear();
    }

    void Flow::pushBack(Element const & element)
    {
        _heap.emplace_back(Slot{element});
        storeIndex(element, std::prev(_heap.end()));
    }

    void Flow::popBack()
    {
        MINIRE_INVARIANT(!_heap.empty(), "cannot pop from empty Flow");
        if (Component const * component = std::get_if<Component>(&_heap.back()._element);
            component)
        {
            erase(component->_id);
        }
        _heap.pop_back();
    }

    void Flow::pushFront(Element const & element)
    {
        _heap.emplace_front(Slot{element});
        storeIndex(element, _heap.begin());
    }

    void Flow::popFront()
    {
        MINIRE_INVARIANT(!_heap.empty(), "cannot pop from empty Flow");
        if (Component const * component = std::get_if<Component>(&_heap.front()._element);
            component)
        {
            erase(component->_id);
        }
        _heap.pop_front();
    }

    void Flow::storeIndex(Element const & element, Heap::iterator iterator)
    {
        if (Component const * component = std::get_if<Component>(&element);
            component)
        {
            auto [_, inserted] = _index.emplace(component->_id, iterator);
            MINIRE_INVARIANT(inserted, "failed to insert into an Index (duplicate?): {}",
                             component->_id);
        }
    }

    void Flow::erase(std::string const & id)
    {
        if (auto it = _index.find(id);
            it != _index.cend())
        {
            assert(it->second != _heap.end());
            _heap.erase(it->second);
            _index.erase(it);
        }
    }
}