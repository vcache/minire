#pragma once

#include <minire/gui/arranger.hpp>
#include <minire/gui/layout.hpp>

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace minire::gui
{
    class Component;
}

namespace minire::gui::layouts
{
    // TODO: add alignment (Begin/Center/End/Justify),
    //       when total size less than a container
    // TODO: add paddings (min/max)
    class Array : public Layout
    {
    public:
        struct Element
        {
            std::optional<std::string> _id;
            Dimension                  _dimension;
        };

        using Elements = std::list<Element>;

        using Sptr = std::shared_ptr<Array>;

    public:
        // horizontal=true is a "row"
        // horizontal=false is a "column"
        explicit Array(bool horizontal,
                       Elements const & elements = {});

        Areas evaluate(Area const & clientArea,
                       Targets const &) const override;


        void onErase(gui::Component const &) override;

        void onClear() override;

        void pushBack(Element const &);
        void pushBack(Dimension dim) { pushBack(Element{std::nullopt, dim}); }
        void pushBack(std::string id, Dimension dim) { pushBack(Element{id, dim}); }
        void pushBack(Component const &, Dimension dim);
        void pushBack(std::shared_ptr<Component> const &, Dimension dim);

        void popBack();

        void pushFront(Element const &);
        void pushFront(Dimension d) { pushFront(Element{std::nullopt, d}); }
        void pushFront(std::string id, Dimension dim) { pushFront(Element{id, dim}); }
        void pushFront(Component const &, Dimension dim);
        void pushFront(std::shared_ptr<Component> const &, Dimension dim);

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

    class Row : public Array
    {
    public:
        explicit Row(Elements const & elements = {})
            : Array(true, elements)
        {}
    };

    class Column : public Array
    {
    public:
        explicit Column(Elements const & elements = {})
            : Array(false, elements)
        {}
    };
}
 