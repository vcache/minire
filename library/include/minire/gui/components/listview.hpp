#pragma once

#include <minire/gui/callbacks.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/components/scrollbar.hpp>
#include <minire/gui/components/text.hpp>
#include <minire/gui/layouts/vertical-tool.hpp>

#include <any>
#include <cassert>
#include <functional>
#include <optional>
#include <vector>

namespace minire::gui::components
{
    namespace listview
    {
        struct OnSelectionChanged
        {
            std::optional<size_t> _previous;
            std::optional<size_t> _current;
        };

        template<typename Stringify>
        class SimpleItemBuilder
        {
        public:
            explicit SimpleItemBuilder(Stringify && stringify)
                : _stringify(std::forward<Stringify>(stringify))
            {}

            Component::Sptr operator()(std::any const & value, size_t const index,
                                       Theme const & theme, Theme::Style const & style,
                                       OverlayController & overlayController) const
            {
                // TODO: cascaded style [listview, text]
                auto result = std::make_shared<Text>("__simple_item__", theme, style,
                                                     overlayController, _stringify(value, index));
                result->horizontal() = Arranger(position::Begin{}, dimension::Fill{});
                result->vertical() = Arranger(position::Center{}, dimension::Content{});
                return result;
            }

        private:
            Stringify _stringify;
        };
    }

    // TODO: optional scrollbar
    // TODO: empty list placeholder content
    // TODO: allow unselect flag
    // TODO: implement smooth scrolling

    class ListView final
        : public Image
        , public Callback<ListView, listview::OnSelectionChanged>
    {
    public:
        using ItemBuilderCallback =
            std::function<Component::Sptr(std::any const & value, size_t index,
                                          Theme const & theme, Theme::Style const & style,
                                          OverlayController & overlayController)>;

        ListView(std::string const & id,
                 Theme const &,
                 Theme::Style const &,
                 OverlayController &,
                 ItemBuilderCallback = {});

        using Sptr = std::shared_ptr<ListView>;
        using Wptr = std::weak_ptr<ListView>;
        using Contents = std::vector<std::any>;
        using Selected = std::optional<size_t>;

        using CommonCallbacks::handle;
        using CommonCallbacks::setCallback;
        using CommonCallbacks::eraseCallback;
        using Callback<ListView, listview::OnSelectionChanged>::handle;
        using Callback<ListView, listview::OnSelectionChanged>::setCallback;
        using Callback<ListView, listview::OnSelectionChanged>::eraseCallback;

        Scrollbar const & scrollbar() const { assert(_scrollbar); return *_scrollbar; }
        Scrollbar & scrollbar() { assert(_scrollbar); return *_scrollbar; }

        Property<minire::models::sprite::MaybeImage> const & background() const { return Image::image(); }
        Property<minire::models::sprite::MaybeImage> & background() { return Image::image(); }

        Property<Contents> const & contents() const { return _contents; }
        Property<Contents> & contents() { return _contents; }

        Property<float> const & lineHeight() const { return _lineHeight; }
        Property<float> & lineHeight() { return _lineHeight; }

        void select(Selected);
        Selected const & selected() const;
        std::any const * current() const;

        template<typename Callback>
        void setItemBuilderCallback(Callback callback)
        {
            _itemBuilderCallback = callback;
            invalidate();
        }

        layouts::VerticalTool const & listViewLayout() const
        {
            assert(_verticalToolLayout);
            return *_verticalToolLayout;
        }

        layouts::VerticalTool & listViewLayout()
        {
            assert(_verticalToolLayout);
            return *_verticalToolLayout;
        }

        void scroll(int deltaY);

        void scrollPageUp();
        void scrollPageDown();
        void scrollHome();
        void scrollEnd();
        void scrollToSelected();

    protected:
        void initialize() override;

        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

    private:
        class ListViewLayout;

        Property<Contents>              _contents;
        Property<size_t>                _offset;
        Property<float>                 _lineHeight;

        VerticalScrollbar::Sptr         _scrollbar;
        Component::Sptr                 _contentContainer;
        std::shared_ptr<ListViewLayout> _contentLayout;
        layouts::VerticalTool::Sptr     _verticalToolLayout;
        ItemBuilderCallback             _itemBuilderCallback;

        Selected                        _selected;
        float                           _heightLimit = 0;
        size_t                          _showLines = 0;
        bool                            _hasIntialScrollDone = false;
    };
}
