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
    public:
        Container(GuiController & controller,
                  std::string const & id,
                  std::shared_ptr<Container> const & parent,
                  std::shared_ptr<Layout> layout = {})
            : Component(controller, std::move(id), parent)
            , _layout(layout ? layout : std::make_shared<Layout>())
        {}

        using Sptr = std::shared_ptr<Container>;

    public:
        template<typename T, typename ... Args>
        std::shared_ptr<T> emplace(std::string const & childId,
                                   Args &&... args)
        {
            auto sharedThis = std::dynamic_pointer_cast<Container>(shared_from_this());
            auto ptr = std::make_shared<T>(_controller, childId, sharedThis,
                                           std::forward<Args>(args)...);
            auto [_, inserted] = _children.emplace(childId, ptr);
            MINIRE_INVARIANT(inserted, "failed to insert parent component: \"{}\" into \"{}\"",
                             childId, id());
            _zOrderStore.emplace(ptr);
            updateChildrenContentArea(ptr);
            return ptr;
        }

        void clear();

        void erase(std::string const & childId);

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

    protected:
        void onVisibleChanged() override;
        void onContentAreaChanged() override;
        size_t onZOrderChanged(size_t offset, ZOrderUpdates & labels,
                               ZOrderUpdates & sprites) override;
        void onChildSubscriptionChanged() override;

        void onEvent(events::application::OnMouseWheel const &) override;
        void onEvent(events::application::OnMouseMove const &) override;
        void onEvent(events::application::OnMouseDown const &) override;
        void onEvent(events::application::OnMouseUp const &) override;
        void onEvent(events::application::OnKeyUp const &) override;
        void onEvent(events::application::OnKeyDown const &) override;
        void onEvent(events::application::OnTextInput const &) override;

    private:
        void updateChildrenContentArea(Component::Sptr const &);

        template<typename T>
        void broadcast(T const & e);

        template<typename T>
        void sendUnderCursor(T const & e);

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
        ZOrderStore      _zOrderStore;
        Layout::Sptr     _layout;

        friend class Component;
    };
}
