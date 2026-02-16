#include <minire/gui/layouts/array.hpp>

#include <minire/errors.hpp>
#include <minire/gui/component.hpp>
#include <minire/logging.hpp>

#include <utils/overloaded.hpp>

#include <cassert>

namespace minire::gui::layouts
{
    Array::Array(bool horizontal,
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

    Layout::Areas Array::evaluate(Area const & clientArea,
                                  Layout::Targets const & targets) const
    {
        // Prefetch size of components based on their content size
        for(Layout::Target const & target : targets)
        {
            gui::Component const & component = target._component;
            if (auto it = _index.find(component.id());
                it != _index.cend())
            {
                assert(it->second != _heap.end());
                Slot & slot = *(it->second);
                slot._offset = 0;

                if (std::holds_alternative<dimension::Content>(slot._element._dimension))
                {
                    std::optional<std::pair<float, float>> const & sz = component.measureContent();
                    MINIRE_INVARIANT(sz, "Array layout requires measurable children, but \"{}\" is not",
                                     component.id());
                    slot._width = sz->first;
                    slot._height = sz->second;
                }
            }
            else
            {
                MINIRE_WARNING("unknown Array layout target: {}", component.id());
            }
        }

        // Measure sizes of isolated targets
        size_t fillCount = 0;
        for(Slot & slot : _heap)
        {
            slot._offset = 0;

            auto [width, height] = std::visit(utils::Overloaded
            {
                [&clientArea, this](dimension::Constant const & v)
                {
                    return _horizontal ? std::make_pair(v._dimension, clientArea._height)
                                       : std::make_pair(clientArea._width, v._dimension);
                },
                [this, &clientArea](dimension::Fraction const & v)
                {
                    return _horizontal ? std::make_pair(clientArea._width * v._fraction, clientArea._height)
                                       : std::make_pair(clientArea._width, clientArea._height * v._fraction);
                },
                [&fillCount](dimension::Fill const &) { fillCount++; return std::make_pair(0.0f, 0.0f); },
                [&slot](dimension::Content const &) { return std::make_pair(slot._width, slot._height); },
            }, slot._element._dimension);

            slot._width = width;
            slot._height = height;
        }

        if (fillCount > 0)
        {
            // Measure sizes residue space
            float residue = _horizontal ? clientArea._width : clientArea._height;
            for(Slot const & slot : _heap)
            {
                residue -= (_horizontal ? slot._width : slot._height);
            }

            if (residue > 0)
            {
                // Measure sizes targets w/ Fill{} dimension
                float const perFillSize = residue / static_cast<float>(fillCount);
                for(Slot & slot : _heap)
                {
                    if (std::holds_alternative<dimension::Fill>(slot._element._dimension))
                    {
                        slot._width = _horizontal  ? perFillSize : clientArea._width;
                        slot._height = _horizontal ? clientArea._height : perFillSize;
                    }
                }
            }
        }

        // Calculate offsets
        float offset = 0;
        for(Slot & slot : _heap)
        {
            slot._offset = offset;
            offset += _horizontal ? slot._width : slot._height;
        }

        // Build the result
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

        // Finish
        return result;
    }

    void Array::onErase(gui::Component const & component)
    {
        erase(component.id());
    }

    void Array::onClear()
    {
        _heap.clear();
        _index.clear();
    }

    void Array::pushBack(Element const & element)
    {
        _heap.emplace_back(Slot{element});
        storeIndex(element, std::prev(_heap.end()));
    }

    void Array::popBack()
    {
        MINIRE_INVARIANT(!_heap.empty(), "cannot pop from empty Array");
        Element const & element = _heap.back()._element;
        if (element._id)
        {
            erase(*element._id);
        }
        _heap.pop_back();
    }

    void Array::pushFront(Element const & element)
    {
        _heap.emplace_front(Slot{element});
        storeIndex(element, _heap.begin());
    }

    void Array::popFront()
    {
        MINIRE_INVARIANT(!_heap.empty(), "cannot pop from empty Array");
        Element const & element = _heap.front()._element;
        if (element._id)
        {
            erase(*element._id);
        }
        _heap.pop_front();
    }

    void Array::storeIndex(Element const & element,
                           Heap::iterator iterator)
    {
        if (element._id)
        {
            auto [_, inserted] = _index.emplace(*element._id, iterator);
            MINIRE_INVARIANT(inserted, "failed to insert into an Index (duplicate?): {}",
                             *element._id);
        }
    }

    void Array::erase(std::string const & id)
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