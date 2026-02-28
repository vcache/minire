#pragma once

#include <minire/gui/callbacks.hpp>
#include <minire/gui/component.hpp>
#include <minire/gui/content-view.hpp>
#include <minire/text/formatted-string.hpp>
#include <minire/text/text-format.hpp>

#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace minire::gui::components
{
    // TODO: min/max length
    // TODO: customize text under selection (fixed color or invert)
    // TODO: use system-wide key combinations (ctrl+c, ctrl+v, etc)
    // TODO: unicode combos
    // TODO: blinking
    // TODO: context menu
    namespace editbox
    {
        struct OnTextChanged
        {
            // TODO: maybe add _previous?
            text::FormattedString const & _current;
        };
    }

    class Editbox final
        : public Component
        , public Callback<Editbox, editbox::OnTextChanged>
    {
    public:
        using Sptr = std::shared_ptr<Editbox>;
        using Wptr = std::weak_ptr<Editbox>;

        using CommonCallbacks::handle;
        using CommonCallbacks::setCallback;
        using CommonCallbacks::eraseCallback;
        using Callback<Editbox, editbox::OnTextChanged>::handle;
        using Callback<Editbox, editbox::OnTextChanged>::setCallback;
        using Callback<Editbox, editbox::OnTextChanged>::eraseCallback;

        using PasswordChar = std::optional<wchar_t>;

        Editbox(std::string const & id,
                Theme const & theme,
                Theme::Style const & style,
                OverlayController &);

        Property<ImageView::Sptr> const & bgNormal() const { return _bgNormal; }
        Property<ImageView::Sptr> & bgNormal() { return _bgNormal; }

        Property<ImageView::Sptr> const & bgDisabled() const { return _bgDisabled; }
        Property<ImageView::Sptr> & bgDisabled() { return _bgDisabled; }

        Property<ImageView::Sptr> const & cursorImageInsert() const { return _cursorImageInsert; }
        Property<ImageView::Sptr> & cursorImageInsert() { return _cursorImageInsert; }

        Property<ImageView::Sptr> const & cursorImageReplace() const { return _cursorImageReplace; }
        Property<ImageView::Sptr> & cursorImageReplace() { return _cursorImageReplace; }

        Property<ImageView::Sptr> const & selectionImage() const { return _selectionImage; }
        Property<ImageView::Sptr> & selectionImage() { return _selectionImage; }

        // true -> insert; false -> replace
        Property<bool> const & insertMode() const { return _insertMode; }
        Property<bool> & insertMode() { return _insertMode; }

        Property<bool> const & enabled() const { return _enabled; }
        Property<bool> & enabled() { return _enabled; }

        Property<PasswordChar> const & passwordChar() const { return _passwordChar; }
        Property<PasswordChar> & passwordChar() { return _passwordChar; }

        Property<utils::Rect> const & contentPadding() const { return _contentPadding; }
        Property<utils::Rect> & contentPadding() { return _contentPadding; }

        text::Format const & activeFormat() const { return _activeFormat; }
        text::Format & activeFormat() { return _activeFormat; }

    public:
        Property<text::FormattedString> const & text() const { return _text; }

        template<typename Fun>
        void editText(Fun fun)
        {
            fun(_text);
            if (_text.isInvalidated())
            {
                revalidatePositions();

                // TODO: it may cause infinite loops:
                //handleTextChange();
            }
        }

        std::wstring toUnicode() const { return _text.get().wunformat(); }
        std::string toUtf8() const { return _text.get().unformat(); }

        void clear();

    protected:
        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override;

        void handle(gui::events::OnFocus const &) override;
        void handle(gui::events::OnUnfocus const &) override;
        void handle(gui::events::OnDragMove const &) override;
        void handle(gui::events::OnDragEnd const &) override;
        void handle(minire::events::application::OnMouseDown const &) override;
        void handle(minire::events::application::OnKeyDown const &) override;
        void handle(minire::events::application::OnTextInput const &) override;

    private:
        void handleTextChange();
        void processInput(std::wstring const &);
        void backspace();
        void de1ete();

        template<typename Pred>
        size_t skipIf(bool forward, Pred) const;

        size_t skipWord(bool forward) const;

        std::optional<size_t> mouseToCursor(float x, float y) const;

    private:
        using MaybeSelection = std::optional<std::pair<size_t, size_t>>;
        using MaybeSelectionText = std::optional<std::string>;

        MaybeSelection getSelection() const;
        MaybeSelectionText getSelectionText() const;
        bool hasSelection() const;
        void startSelection();
        void dropSelection();
        bool eraseSelection(); // true iif smth was erased
        void actualizePrimarySelection();

        void revalidatePositions();

    private:
        static constexpr size_t kNoIndex = std::numeric_limits<size_t>::max();

        Property<ImageView::Sptr>       _bgNormal;
        Property<ImageView::Sptr>       _bgDisabled;
        Property<ImageView::Sptr>       _cursorImageInsert;
        Property<ImageView::Sptr>       _cursorImageReplace;
        Property<ImageView::Sptr>       _selectionImage;
        Property<bool>                  _insertMode;
        Property<bool>                  _enabled;
        Property<PasswordChar>          _passwordChar;
        Property<utils::Rect>           _contentPadding;

        Property<text::FormattedString> _text;
        TextView::Sptr                  _textView;
        text::Format                    _activeFormat;
        size_t                          _cursorPos; //  TODO: make it Property (public) + avoid invalidateContent()s
        size_t                          _selectionBegin;
        float                           _offset;
        MaybeSelection                  _primarySelection;
        glm::vec2                       _contentAreaOffset;
    };
}
