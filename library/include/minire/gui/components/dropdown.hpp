#pragma once

#include <minire/gui/arranger.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/components/container.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/components/scrollbar.hpp>
#include <minire/utils/rect.hpp>

#include <any>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace minire::gui::components
{
    class Dropdown final
        : public Container
    {
    public:
        using Sptr = std::shared_ptr<Dropdown>;
        using Wptr = std::weak_ptr<Dropdown>;

        Dropdown(GuiController & controller,
                 std::string const & id,
                 std::shared_ptr<Container> const & parent);

        struct Background
        {
            content::Id      _texture;
            utils::NinePatch _patch;
        };

        // NOTE: It must be called right after a ctor.
        //       This is workaround for shared_from_this() from a ctor
        //       problem (cannot call this->emplace() from a ctor).
        // TODO: fix it!!
        void init(Background const & baseBackground,
                  Background const & tongueBackground,
                  Button::Background const & buttonBackground,
                  Button::MaybeIcon const & buttonIcon = std::nullopt,
                  Button::MaybeText const & buttonText = std::nullopt,
                  Arrangers arrangers = Arrangers(),
                  float tongueMaxHeight = 200,
                  size_t tongueMaxLines = 5,
                  std::optional<size_t> constantLineHeight = std::nullopt);

    public:
        Button & button() { assert(_dropButton); return *_dropButton; }
        Button const & button() const { assert(_dropButton); return *_dropButton; }

        enum class Purpose
        {
            kActiveLine, kTongueLine
        };

        using ItemBuilderCallback =
            std::function<Button::Sptr(std::any const &, size_t, bool, Purpose)>;

        template<typename Callback>
        void setItemBuilderCallback(Callback callback)
        {
            _itemBuilderCallback = callback;
            refillOverlay();
        }

        using ScrollbarBuilderCallback = std::function<Scrollbar::Sptr()>;

        template<typename Callback>
        void setScrollbarBuilderCallback(float const width, Callback callback)
        {
            _scrollbarWidth = width;
            _scrollbarBuilderCallback = callback;
            refillOverlay();
        }

        using SelectionChangedCallback =
            std::function<void(Dropdown &,
                               std::optional<size_t> previous,
                               std::optional<size_t> current)>;

        template<typename Callback>
        void setSelectionChangedCallback(Callback callback)
        {
            _selectionChangedCallback = callback;
        }

    public:
        std::vector<std::any> const & contents() const { return _contents; }

        template<typename UnaryOp>
        void editContents(UnaryOp unaryOp)
        {
            if (unaryOp(_contents))
                revalidateContents();
        }

        std::any const * selectedValue() const;

        std::optional<size_t> selectedIndex() const { return _selectedIndex; }

        void select(std::optional<size_t>);

        void destroyOverlay(); // close the tongue if any

    protected:
        void onContentAreaChanged() override;

    private:
        void buildOverlay();
        void refillOverlay();
        void rearrangeTongue();
        void revalidateContents();
        void wheelScroll(int deltaY);

    private:
        class DefaultHandler;
        class TongueLayout;

        struct Tongue
        {
            std::string                     _tag;
            std::shared_ptr<DefaultHandler> _defaultHandler;
            Container::Sptr                 _container;
            Scrollbar::Wptr                 _scrollbar;
            size_t                          _offset;
            std::vector<Component::Wptr>    _subButtons;
        };

        Background               _tongueBackground;
        float                    _tongueMaxHeight = 0;
        size_t                   _tongueMaxLines = 0;
        std::optional<size_t>    _constantLineHeight;
        Image::Sptr              _background;
        Button::Sptr             _dropButton;
        Button::Wptr             _activeItem;
        float                    _scrollbarWidth = 0;
        utils::Rect              _activeItemPaddings = utils::Rect(1.0f);
        utils::Rect              _tonguePaddings = utils::Rect(1.0f);
        std::unique_ptr<Tongue>  _tongue;
        ItemBuilderCallback      _itemBuilderCallback;
        SelectionChangedCallback _selectionChangedCallback;
        ScrollbarBuilderCallback _scrollbarBuilderCallback;

        std::vector<std::any>    _contents;
        std::optional<size_t>    _selectedIndex;

        float                    _expectedTongueHeight = 0;
        bool                     _inited = false;

        friend class DefaultHandler;
        friend class TongueLayout;
    };
}
