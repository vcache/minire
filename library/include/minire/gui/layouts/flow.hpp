#pragma once

#include <minire/gui/layout.hpp>

#include <list>
#include <string>
#include <unordered_map>
#include <variant>

namespace minire::gui::layouts
{
    class Flow : public Layout
    {
    public:
        struct Component
        {
            std::string _id;
        };

        struct Spacing
        {
            float _value;
        };

        using Element = std::variant<Component, Spacing>;

    public:
        // horizontal=true is a "row"
        // horizontal=false is a "column"

        explicit Flow(bool horizontal,
                      std::list<Element> const & elements = {});

        Areas evaluate(Area const & clientArea,
                       Targets const &) const override;

        void onErase(gui::Component const &) override;

        void onClear() override;

        void pushBack(Element const &);

        void popBack();

        void pushFront(Element const &);

        void popFront();

        size_t size() const { return _heap.size(); }

    private:
        struct Slot
        {
            Element _element;
            float   _offset = 0;
            float   _width = 0;
            float   _height = 0;
        };

        using Heap = std::list<Slot>;
        using Index = std::unordered_map<std::string, Heap::iterator>;

        mutable Heap _heap;
        Index        _index;
        bool const   _horizontal;

        void storeIndex(Element const &, typename Heap::iterator);
        void erase(std::string const &);
    };
}