#pragma once

#include <minire/gui/callbacks.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/components/listview.hpp>
#include <minire/gui/content-view.hpp>
#include <minire/gui/layouts/vertical-tool.hpp>

#include <any>
#include <cassert>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace minire::gui::components
{
    namespace dropdown
    {
        struct OnSelectionChanged
        {
            std::optional<size_t> _previous;
            std::optional<size_t> _current;
        };
    }

    // TODO: dont' open tongue on empty list
    class Dropdown final
        : public Component
        , public Callback<Dropdown, dropdown::OnSelectionChanged>
    {
    public:
        using ItemBuilderCallback =
            std::function<ContentView::Sptr(std::any const & value, size_t index)>;

        Dropdown(std::string const & id,
                 Theme const & theme,
                 OverlayController &,
                 ItemBuilderCallback baseItemBuilder = {},
                 ItemBuilderCallback tongueItemBuilder = {});

        ~Dropdown() override;

        using Sptr = std::shared_ptr<Dropdown>;
        using Wptr = std::weak_ptr<Dropdown>;
        using Contents = std::vector<std::any>;
        using Selected = std::optional<size_t>;

        using CommonCallbacks::handle;
        using CommonCallbacks::setCallback;
        using CommonCallbacks::eraseCallback;
        using Callback<Dropdown, dropdown::OnSelectionChanged>::handle;
        using Callback<Dropdown, dropdown::OnSelectionChanged>::setCallback;
        using Callback<Dropdown, dropdown::OnSelectionChanged>::eraseCallback;

        Button const & dropButton() const { assert(_dropButton); return *_dropButton; }
        Button & dropButton() { assert(_dropButton); return *_dropButton; }

        Property<ImageView::Sptr> const & background() const { return _background; }
        Property<ImageView::Sptr> & background() { return _background; }

        Property<theme::Dropdown::Constants::Tongue> const & tongue() const { return _tongue; }
        Property<theme::Dropdown::Constants::Tongue> & tongue() { return _tongue; }

        Property<Contents> const & contents() const { return _contents; }
        Property<Contents> & contents() { return _contents; }

        Property<float> const & lineHeight() const { return _lineHeight; }
        Property<float> & lineHeight() { return _lineHeight; }

        layouts::VerticalTool const & dropDownLayout() const
        {
            assert(_dropdownLayout);
            return *_dropdownLayout;
        }

        layouts::VerticalTool & dropDownLayout()
        {
            assert(_dropdownLayout);
            return *_dropdownLayout;
        }

        void select(Selected);
        Selected const & selected() const;
        std::any const * current() const;

        // This is is mandatory.
        template<typename Callback>
        void setBaseItemBuilder(Callback callback)
        {
            _baseItemBuilderCallback = callback;
            invalidate();
        }

        // This one is optional. When omitted,
        // BaseItemBuilder will be used.
        template<typename Callback>
        void setTongueItemBuilder(Callback callback)
        {
            _tongueItemBuilderCallback = callback;
            invalidate();
        }

        void handle(gui::events::OnClick const &) override;

    protected:
        void initialize() override;

        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & clientArea) override;
    private:
        void openTongue();
        void closeTongue();
        void wheelScroll(int deltaY);

        class DefaultHandler;

        struct TongueOverlay
        {
            std::string const               _tag;
            std::shared_ptr<DefaultHandler> _defaultHandler;
            std::shared_ptr<ListView>       _listview;
            bool                            _destroy;
        };

    private:
        using Tongue = theme::Dropdown::Constants::Tongue;

        Property<ImageView::Sptr>      _background;
        Property<Tongue>               _tongue;
        Property<Contents>             _contents;
        Property<float>                _lineHeight;

        Button::Sptr                   _dropButton;
        layouts::VerticalTool::Sptr    _dropdownLayout;
        ItemBuilderCallback            _baseItemBuilderCallback;
        ItemBuilderCallback            _tongueItemBuilderCallback;

        std::unique_ptr<TongueOverlay> _tongueOverlay;
        Selected                       _selected;
        ContentView::Sptr              _activeItem;

        friend class DefaultHandler;
    };
}
