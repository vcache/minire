#pragma once

#include <minire/gui/content-view.hpp>
#include <minire/text/text-format.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

#include <memory>
#include <optional>

namespace minire::gui
{
    class Component;

    // TODO: Styles into a Theme (i.e. different styles of buttons, scrollbars, etc)

    namespace theme
    {
        enum class Location
        {
            kLeft, kTop, kRight, kBottom,
        };

        enum class Icon
        {
            kArrowLeft,
            kArrowUp,
            kArrowRight,
            kArrowDown,
            kDecrease,
            kIncrease,
        };

        class Button
        {
        public:
            struct Constants
            {
                Location    _iconLocation;
                float       _iconSpacing;
                utils::Rect _padding;
                glm::vec2   _pressOffset;
            };

        public:
            explicit Button(Constants const & constants)
                : _constants(constants)
            {}

            virtual ~Button() = default;

            Constants const & constants() const { return _constants; }

            virtual ImageView::Sptr makeNormalBg() const = 0;
            virtual ImageView::Sptr makeHoveredBg() const = 0;
            virtual ImageView::Sptr makePressedBg() const = 0;

        private:
            Constants _constants;
        };

        class Scrollbar
        {
        public:
            struct Constants
            {
                float _minSliderLength;
            };

            explicit Scrollbar(Constants const & constants)
                : _constants(constants)
            {}

            virtual ~Scrollbar() = default;

            Constants const & constants() const { return _constants; }

            virtual ImageView::Sptr makeBackground() const = 0;

        private:
            Constants _constants;
        };

        class ListView
        {
        public:
            struct Constants
            {
                utils::Rect _padding;
                float       _scrollbarWidth;
                bool        _scrollbarAtLeft;
            };

            explicit ListView(Constants const & constants)
                : _constants(constants)
            {}

            virtual ~ListView() = default;

            Constants const & constants() const { return _constants; }

            virtual ImageView::Sptr makeBackground() const = 0;
            virtual ImageView::Sptr makeNormalItemBackground() const = 0;
            virtual ImageView::Sptr makeHoverItemBackground() const = 0;
            virtual ImageView::Sptr makeSelectedItemBackground() const = 0;

        private:
            Constants _constants;
        };

        class Dropdown
        {
        public:
            struct Constants
            {
                struct Tongue
                {
                    std::optional<size_t> _maxLines;
                    std::optional<float>  _minHeight;
                    std::optional<float>  _maxHeight;
                };

                utils::Rect _padding;
                Tongue      _tongue;
                float       _dropButtonWidth;
                bool        _dropButtonAtLeft;
            };

            explicit Dropdown(Constants const & constants)
                : _constants(constants)
            {}

            virtual ~Dropdown() = default;

            Constants const & constants() const { return _constants; }

            virtual ImageView::Sptr makeBackground() const = 0;
            virtual theme::ListView const & tongue() const = 0;

        private:
            Constants _constants;
        };

        class Editbox
        {
        public:
            struct Constants
            {
                utils::Rect  _contentPadding;
                text::Format _activeFormat;
            };

        public:
            explicit Editbox(Constants const & constants)
                : _constants(constants)
            {}

            virtual ~Editbox() = default;

            Constants const & constants() const { return _constants; }

            virtual ImageView::Sptr makeNormalBg() const = 0;

            virtual ImageView::Sptr makeDisabledBg() const = 0;

            virtual ImageView::Sptr makeCursorImageInsert() const = 0;

            virtual ImageView::Sptr makeCursorImageReplace() const = 0;

            virtual ImageView::Sptr makeSelectionImage() const = 0;

        private:
            Constants _constants;
        };
    }

    class Theme
    {
    public:
        virtual ~Theme() = default;

        virtual ImageView::Sptr makeIcon(theme::Icon) const = 0;

        virtual theme::Button const & button() const = 0;
        virtual theme::Scrollbar const & scrollbar() const = 0;
        virtual theme::ListView const & listview() const = 0;
        virtual theme::Dropdown const & dropdown() const = 0;
        virtual theme::Editbox const & editbox() const = 0;
    };
}
