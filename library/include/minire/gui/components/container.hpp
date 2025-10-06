#pragma once

#include <minire/errors.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/layout.hpp>

#include <cassert>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>

namespace minire::gui::components
{
    class Container
        : public Component
    {
        // NOTE: don't call it from the ctor!
        std::shared_ptr<Container> sharedThis()
        {
            return std::dynamic_pointer_cast<Container>(shared_from_this());
        }

    public:
        Container(GuiController & controller,
                  std::string const & id,
                  std::shared_ptr<Container> const & parent,
                  Layout::Sptr const & layout = {})
            : Component(controller, std::move(id), parent)
            , _layout(layout ? layout : std::make_shared<Layout>())
        {
            // TODO: avoid call to sharedThis() and store weak_ptr into a Layout
            _layout->setParent(*this);
        }

        using Sptr = std::shared_ptr<Container>;
        using Wptr = std::weak_ptr<Container>;

    public:
        void emplace(std::shared_ptr<Component> const &);

        template<typename T, typename ... Args>
        std::shared_ptr<T> emplace(std::string const & childId,
                                   Args &&... args)
        {
            auto child = std::make_shared<T>(_controller, childId, sharedThis(),
                                             std::forward<Args>(args)...);
            emplaceImpl(child);
            return child;
        }

        void clear();

        void erase(std::string const & childId);

    public:
        Layout & layout() { assert(_layout); return *_layout; }

        Layout const & layout() const { assert(_layout); return *_layout; }

    public:
        bool contains(std::string const & childId) const { return _children.contains(childId); }

        template<typename T>
        std::shared_ptr<T> const & find(std::string const & childId) const
        {
            auto it = _children.find(childId);
            return it != _children.cend() ? std::dynamic_pointer_cast<T>(it->second)
                                          : std::shared_ptr<T>();
        }

        template<typename T>
        std::shared_ptr<T> find(std::string const & childId)
        {
            return find<T>(childId);
        }

        template<typename T>
        T const & at(std::string const & childId) const
        {
            auto it =_children.find(childId);
            MINIRE_INVARIANT(it != _children.cend(),
                             "no such component: \"{}\" in a container \"{}\"",
                             childId, id());
            return dynamic_cast<T &>(*it->second);
        }

        template<typename T>
        T & at(std::string const & childId)
        {
            return const_cast<T &>(const_cast<Container const *>(this)->at<T>(childId));
        }

        Component::Sptr findUnderCursor(float x, float y);

    protected:
        void onVisibleChanged() override;
        void onContentAreaChanged() override;
        size_t onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                               ZOrderUpdates & sprites) override;

    private:
        void updateChildrenContentArea(Component::Sptr const &);

        void emplaceImpl(std::shared_ptr<Component> const & child);

    private:
        struct ZOrderCompare
        {
            bool operator()(Component::Sptr const & lhs,
                            Component::Sptr const & rhs) const
            {
                assert(lhs);
                assert(rhs);

                if (lhs->zOrder() < rhs->zOrder()) return true;
                if (lhs->zOrder() > rhs->zOrder()) return false;

                return lhs.get() < rhs.get();
            }
        };

        using ZOrderStore = std::set<Component::Sptr, ZOrderCompare>;
        using Children = std::unordered_map<std::string, Component::Sptr>;

        Children         _children;
        ZOrderStore      _zOrderStore;  // NOTE: this is "logical zOrder"
                                        // onZOrderChanged calcs "physical zOrder"
        Layout::Sptr     _layout;

        friend class Component;
    };
}
