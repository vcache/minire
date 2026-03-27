#include <minire/gui/components/editbox.hpp>

#include <minire/gui/overlay-controller.hpp>
#include <minire/logging.hpp>
#include <minire/text/unicode.hpp>
#include <minire/utils/glyph-grid.hpp>

#include <glm/common.hpp> // for glm::clamp

#include <cwctype>

namespace minire::gui::components
{
    namespace
    {
        // TODO: won't work w/o std::setlocale
        bool isNonWalnum(wchar_t c)
        {
            return !std::iswalnum(c);
        }

        // Ensure that "mod" has any bit from the "mask",
        // and no other bits are present
        bool testMask(uint16_t const mod,
                      uint16_t const mask)
        {
            return (mod & mask) && ((mod | mask) == mask);
        }

        bool testCombo(uint16_t const mod,
                       uint16_t const mask1,
                       uint16_t const mask2)
        {
            return testMask(mod, mask1 | mask2) &&
                   (mod & mask1) &&
                   (mod & mask2);
        }
    }

    Editbox::Editbox(std::string const & id,
                     Theme const & theme,
                     Theme::Style const & style,
                     OverlayController & overlayController)
        : Component(id, theme, style, overlayController)
        , _bgNormal(std::make_shared<Image>("__bg-normal__", theme, style, overlayController,
                    theme.get<models::sprite::MaybeImage>(kName, "bg-normal", style)))
        , _bgDisabled(std::make_shared<Image>("__bg-disabled__", theme, style, overlayController,
                      theme.get<models::sprite::MaybeImage>(kName, "bg-disabled", style)))
        , _cursorImageInsert(std::make_shared<Image>("__cursor-insert__", theme, style, overlayController,
                             theme.get<models::sprite::MaybeImage>(kName, "cursor-insert", style)))
        , _cursorImageReplace(std::make_shared<Image>("__cursor-replace__", theme, style, overlayController,
                              theme.get<models::sprite::MaybeImage>(kName, "cursor-replace", style)))
        , _selectionImage(std::make_shared<Image>("__selection__", theme, style, overlayController,
                          theme.get<models::sprite::MaybeImage>(kName, "selection-bg", style)))
        , _insertMode(*this, true)
        , _enabled(*this, true)
        , _passwordChar(*this, std::nullopt)
        , _contentPadding(*this, theme.get<utils::Rect>(kName, "content-padding", style))
        , _text(*this)
        , _textView(std::make_shared<Text>("__text__", theme, concat(style, kName), overlayController))
        , _activeFormat(theme.get<text::Format>(kName, "active-format", style))
        , _cursorPos(0)
        , _selectionBegin(kNoIndex)
        , _offset(0)
        , _primarySelection(std::nullopt)
        , _contentAreaOffset(0)
    {
        setSystemCursor(models::SystemCursor::kIbeam);
        setAcceptFocus(true);
        isDraggable() = true;
    }

    void Editbox::clear()
    {
        _text->clear();
        _cursorPos = 0;
        _selectionBegin = kNoIndex;
        _offset = 0;
        _insertMode = true;

        assert(_textView);
        _textView->text()->clear();
    }

    void Editbox::initialize()
    {
        auto sharedThis = shared_from_this();
        // NOTE: adding to a parent() w.r.t. zOrder (bg->selection->text->cursor)

        assert(_bgNormal);
        _bgNormal->setParent(sharedThis);
        _bgNormal->setEventTransparent(true);

        assert(_bgDisabled);
        _bgDisabled->setParent(sharedThis);
        _bgDisabled->setEventTransparent(true);

        assert(_selectionImage);
        _selectionImage->setParent(sharedThis);
        _selectionImage->setEventTransparent(true);

        assert(_textView);
        _textView->setParent(sharedThis);
        _textView->setEventTransparent(true);

        assert(_cursorImageInsert);
        _cursorImageInsert->setParent(sharedThis);
        _cursorImageInsert->setEventTransparent(true);

        assert(_cursorImageReplace);
        _cursorImageReplace->setParent(sharedThis);
        _cursorImageReplace->setEventTransparent(true);
    }

    size_t Editbox::revalidateContent(size_t zOffset,
                                      bool const effectiveVisible,
                                      Area const & contentArea,
                                      Area const & /*clippingWindow*/)
    {
        // calculate content padding
        utils::Rect const & contentPadding = _contentPadding.get();
        Area const realContentArea
        {
            ._left = contentArea._left + contentPadding._left,
            ._top = contentArea._top + contentPadding._top,
            ._width = contentArea._width - contentPadding._left - contentPadding._right,
            ._height = contentArea._height - contentPadding._top - contentPadding._bottom,
        };

        _contentAreaOffset = glm::vec2(realContentArea._left,
                                       realContentArea._top);

        // revalidate background
        if (_enabled.isInvalidated())
        {
            assert(_bgNormal);
            assert(_bgDisabled);
            _bgNormal->visible() = _enabled.get();
            _bgDisabled->visible() = !_enabled.get();
        }

        // maybe rebuild text view from
        if (_text.isInvalidated() ||
            _passwordChar.isInvalidated())
        {
            text::FormattedString const * effectiveText = &(_text.get());

            text::FormattedString substituteText;
            if (_passwordChar.get())
            {
                std::wstring substitute(_text.get().size(), *(_passwordChar.get()));
                substituteText = text::FormattedString(substitute, _activeFormat);
                effectiveText = &substituteText;
            }

            assert(effectiveText);
            assert(_textView);
            _textView->text() = *effectiveText;
            _offset = 0;
        }

        // fetch length of text's prefix (unitl the cursor)
        float prefixSize = 0;
        if (!_text.get().empty())
        {
            assert(_textView);
            utils::TextLayout const & textLayout = _textView->textLayout();
            assert(!textLayout.empty());
            prefixSize = _cursorPos != 0 ? textLayout.layoutOf(_cursorPos - 1)._right : 0;
        }

        // revalidate selection
        {
            assert(_selectionImage);
            _selectionImage->visible() = hasSelection();
            if (_selectionImage->visible().get() && !_text.get().empty())
            {
                assert(_textView);
                utils::TextLayout const & textLayout = _textView->textLayout();
                assert(!textLayout.empty());
                float const selectionBeginSize =
                    _selectionBegin != 0 ? textLayout.layoutOf(_selectionBegin - 1)._right : 0;

                float selectionLeft = std::min(prefixSize, selectionBeginSize);
                float selectionRight = std::max(prefixSize, selectionBeginSize);

                _selectionImage->horizontal() = Arranger(
                    position::Constant{contentPadding._left + selectionLeft + _offset},
                    dimension::Constant{selectionRight - selectionLeft + 1});
                _selectionImage->vertical() = Arranger(
                    position::Constant{contentPadding._top + 1},
                    dimension::Constant{realContentArea._height - 1 - 1}); // TODO: what if _height<2 ?
            }
        }

        // revalidate text view
        if (effectiveVisible)
        {
            assert(_textView);
            utils::Rect const & aabb = _textView->textLayout().aabb();
            glm::vec2 const fullSize(aabb._right - aabb._left,
                                     aabb._bottom - aabb._top);

            float const pivot = prefixSize + _offset;
            if (pivot >= realContentArea._width)
            {
                _offset -= (pivot - realContentArea._width);
            }
            else if (pivot < 0)
            {
                _offset -= pivot;
            }

            _offset = glm::clamp(_offset, -fullSize.x, 0.0f);
            _textView->horizontal() = Arranger(
                position::Constant{contentPadding._left + _offset},
                dimension::Constant{fullSize.x});
            _textView->vertical() = Arranger(
                position::Constant{contentPadding._top + (realContentArea._height - fullSize.y) *.5f},
                dimension::Constant{realContentArea._height});
        }

        // hide inactive cursor image
        if (_insertMode.isInvalidated())
        {
            assert(_cursorImageReplace);
            assert(_cursorImageInsert);
            _cursorImageReplace->visible() = !_insertMode.get();
            _cursorImageInsert->visible() = _insertMode.get();
        }

        // revalidate active cursor image
        {
            auto const & activeCursor = _insertMode.get() ? _cursorImageInsert
                                                          : _cursorImageReplace;
            assert(activeCursor);

            activeCursor->visible() = hasFocus();
            if (activeCursor->visible().get())
            {
                float const pivot = prefixSize + _offset;
                float const width = _insertMode.get() ? 7 : 7;
                // TODO: maybe customize horizontal padding (and width) for the cursor?
                activeCursor->horizontal() = Arranger(
                    position::Constant{contentPadding._left + pivot - (width * .5f)}, // TODO: must be relative
                    dimension::Constant{width});
                activeCursor->vertical() = Arranger(
                    position::Constant{4},
                    dimension::Constant{contentArea._height - 4 - 4});
            }
        }

        // finish
        _insertMode.revalidate();
        _enabled.revalidate();
        _passwordChar.revalidate();
        _contentPadding.revalidate();
        _text.revalidate();

        return zOffset;
    }

    void Editbox::handleTextChange()
    {
        handle(editbox::OnTextChanged{_text.get()});
    }

    void Editbox::processInput(std::wstring const & input)
    {
        if (input.empty())
            return;

        eraseSelection();

        text::FormattedString & buffer = *_text;
        for(wchar_t symbol : input)
        {
            // NOTE: linefeeds will break TextLayout!
            //       See comment near text::layout(...).
            if (text::isLineBreak(symbol))
            {
                symbol = L'\u240A';
            }

            assert(_cursorPos <= buffer.size());
            if (_cursorPos == buffer.size())
            {
                // for both insert and replace mode
                buffer.append(symbol, _activeFormat);
            }
            else if (_insertMode.get())
            {
                buffer.insert(_cursorPos, symbol, _activeFormat);
            }
            else
            {
                buffer[_cursorPos] = text::Symbol(symbol, _activeFormat);
            }
            _cursorPos++;
            assert(_cursorPos <= buffer.size());
        }

        handleTextChange();
        actualizePrimarySelection();
    }

    void Editbox::backspace()
    {
        if (_cursorPos != 0)
        {
            text::FormattedString & buffer = *_text;
            assert(_cursorPos <= buffer.size());
            buffer.erase(_cursorPos - 1);
            _cursorPos--;
            handleTextChange();
            actualizePrimarySelection();
       }
    }

    void Editbox::de1ete()
    {
        if (_cursorPos != _text.get().size())
        {
            text::FormattedString & buffer = *_text;
            assert(_cursorPos < buffer.size());
            buffer.erase(_cursorPos);
            handleTextChange();
            actualizePrimarySelection();
        }
    }

    template<typename Pred>
    size_t Editbox::skipIf(bool forward, Pred pred) const
    {
        text::FormattedString const & buffer = _text.get();

        if (buffer.empty() ||
            (!forward && 0 == _cursorPos) ||
            (forward && buffer.size() == _cursorPos))
        {
            return _cursorPos;
        }

        size_t const last = forward ? buffer.size() : 0;
        int const delta = forward ? 1 : -1;
        for(size_t i = _cursorPos; i != last; i += delta)
        {
            if (!pred(buffer[forward ? i : i - 1].codePoint()))
                return i;
        }

        return last;
    }

    size_t Editbox::skipWord(bool forward) const
    {
        if (size_t const newPos = skipIf(forward, isNonWalnum);
            newPos != _cursorPos)
        {
            return newPos;
        }
        return skipIf(forward, std::iswalnum);
    }

    Editbox::MaybeSelection Editbox::getSelection() const
    {
        if (!hasSelection())
        {
            return std::nullopt;
        }

        return _selectionBegin < _cursorPos
            ? std::make_pair(_selectionBegin, _cursorPos)
            : std::make_pair(_cursorPos, _selectionBegin);
    }

    Editbox::MaybeSelectionText Editbox::getSelectionText() const
    {
        if (auto const & selection = getSelection();
            selection)
        {
            return _text.get().unformat(selection->first,
                                        selection->second);
        }
        return std::nullopt;
    }

    bool Editbox::hasSelection() const
    {
        return _selectionBegin != kNoIndex
            && _selectionBegin != _cursorPos;
    }

    void Editbox::startSelection()
    {
        assert(!hasSelection());
        if (_selectionBegin != _cursorPos)
        {
            _selectionBegin = _cursorPos;
            actualizePrimarySelection();
            invalidateContent();
        }
    }

    void Editbox::dropSelection()
    {
        if (hasSelection())
        {
            actualizePrimarySelection();
            invalidateContent();
        }
        _selectionBegin = kNoIndex;
    }

    bool Editbox::eraseSelection()
    {
        bool erased = false;
        if (auto selection = getSelection();
            selection)
        {
            assert(selection->first < selection->second);
            _text->erase(selection->first, selection->second);
            _cursorPos = selection->first;
            _cursorPos = std::min(_cursorPos, _text.get().size());
            erased = true;
        }

        dropSelection();

        if (erased)
        {
            handleTextChange();
            actualizePrimarySelection();
        }

        return erased;
    }

    void Editbox::actualizePrimarySelection()
    {
        if (_passwordChar.get())
            return;

        if (auto selection = getSelection();
            selection != _primarySelection)
        {
            if (selection)
            {
                MaybeSelectionText const & selectionText = getSelectionText();
                assert(selectionText);
                overlayController().setPrimarySelection(*selectionText);
            }

            _primarySelection = selection;
        }
    }

    void Editbox::revalidatePositions()
    {
        size_t const textSize = _text.get().size();
        _cursorPos = std::min(_cursorPos, textSize);
        if (hasSelection())
        {
            _selectionBegin = std::min(_selectionBegin, textSize);
        }
        actualizePrimarySelection();
    }

    std::optional<size_t> Editbox::mouseToCursor(float x, float y) const
    {
        if (_textView && !_text.get().empty())
        {
            utils::TextLayout const & textLayout = _textView->textLayout();
            utils::Rect const & aabb = textLayout.aabb();
            assert(!textLayout.empty());

            // move according to an _offset
            x = x - _contentAreaOffset.x - _offset;

            // ignore real 'y' to allow out-of-boundaries draging
            y = (aabb._bottom - aabb._top) * .5f;  //y - _contentAreaOffset.y;

            if (std::optional<size_t> index = textLayout.indexOf(x, y); index)
            {
                assert(*index + 1 <= _text.get().size());
                return *index + 1;
            }
            else if (x > aabb._right)
            {
                return _text.get().size();
            }
            else
            {
                return 0;
            }
        }

        return std::nullopt;
    }

    void Editbox::handle(gui::OnFocus const & e)
    {
        overlayController().startTextInput();
        invalidateContent();
        CommonCallbacks::handle(e);
    }

    void Editbox::handle(gui::OnUnfocus const & e)
    {
        dropSelection();
        overlayController().stopTextInput();
        invalidateContent();
        CommonCallbacks::handle(e);
    }

    void Editbox::handle(gui::OnDragMove const & e)
    {
        auto const & mouseMove = e._event;
        if (std::optional<size_t> cursorPos = mouseToCursor(mouseMove._absX,
                                                            mouseMove._absY);
            cursorPos)
        {
            if (!hasSelection())
                startSelection();

            _cursorPos = *cursorPos;
            invalidateContent();
        }

        CommonCallbacks::handle(e);
    }

    void Editbox::handle(application::OnMouseDown const & e)
    {
        bool const isSecret = _passwordChar.get() != std::nullopt;

        if (models::MouseButton::kLeft == e._mouseButton ||
            models::MouseButton::kMiddle == e._mouseButton)
        {
            dropSelection();

            if (std::optional<size_t> cursorPos = mouseToCursor(e._x, e._y);
                cursorPos)
            {
                _cursorPos = *cursorPos;
                assert(_cursorPos <= _text.get().size());
                invalidateContent();
            }

            if (_enabled.get() &&
                models::MouseButton::kMiddle == e._mouseButton)
            {
                assert(!hasSelection());
                if (std::string const & primarySelection = overlayController().primarySelection();
                    !primarySelection.empty())
                {
                    processInput(text::toUnicode(primarySelection));
                }
            }
            else if (e._doubleClick)
            {
                if (!isSecret)
                {
                    _cursorPos = skipWord(false);
                    startSelection();
                    _cursorPos = skipWord(true);
                }
                else
                {
                    _cursorPos = 0;
                    startSelection();
                    _cursorPos = _text.get().size();
                }
                invalidateContent();
            }
        }

        CommonCallbacks::handle(e);
    }

    void Editbox::handle(gui::OnDragEnd const & e)
    {
        actualizePrimarySelection();
        CommonCallbacks::handle(e);
    }

    void Editbox::handle(application::OnKeyDown const & e)
    {
        bool const isSecret = _passwordChar.get() != std::nullopt;
        switch(e._key)
        {
            case SDLK_BACKSPACE:
                if (!_enabled.get())
                    break;
                if (!e._mod)
                {
                    if (!eraseSelection())
                    {
                        backspace();
                    }
                }
                else if (testMask(e._mod, KMOD_CTRL) && !isSecret)
                {
                    if (!eraseSelection() && _cursorPos != 0)
                    {
                        size_t const begin = skipWord(false);
                        _text->erase(begin, _cursorPos);
                        _cursorPos = begin;
                        handleTextChange();
                        invalidateContent();
                    }
                }
                else if (testCombo(e._mod, KMOD_CTRL, KMOD_SHIFT) && !isSecret)
                {
                    if (_cursorPos != 0)
                    {
                        _text->erase(0, _cursorPos);
                        _cursorPos = 0;
                        handleTextChange();
                        invalidateContent();
                    }
                }
                actualizePrimarySelection();
                break;

            case SDLK_DELETE:
                if (!_enabled.get())
                    break;
                if (!e._mod)
                {
                    if (!eraseSelection())
                    {
                        de1ete();
                    }
                }
                else if (testMask(e._mod, KMOD_CTRL) && !isSecret)
                {
                    if (!eraseSelection() &&
                        _cursorPos != _text.get().size())
                    {
                        size_t const end = skipWord(true);
                        _text->erase(_cursorPos, end);
                        handleTextChange();
                        invalidateContent();
                    }
                }
                else if (testCombo(e._mod, KMOD_CTRL, KMOD_SHIFT) && !isSecret)
                {
                    if (_cursorPos != _text.get().size())
                    {
                        _text->erase(_cursorPos, _text.get().size());
                        handleTextChange();
                        invalidateContent();
                    }
                }
                actualizePrimarySelection();
                break;

            case SDLK_LEFT:
                if (bool const hasShift = testMask(e._mod, KMOD_SHIFT);
                    hasShift || !e._mod)
                {
                    if (hasShift && !hasSelection())
                        startSelection();

                    if (!hasShift && hasSelection())
                    {
                        dropSelection();
                    }
                    else if (_cursorPos > 0)
                    {
                        _cursorPos -= 1;
                        invalidateContent();
                    }
                }
                else if (testMask(e._mod, KMOD_CTRL) ||
                         testMask(e._mod, KMOD_ALT))
                {
                    _cursorPos = !isSecret ? skipWord(false) : 0;
                    assert(_cursorPos <= _text.get().size());
                    invalidateContent();
                }
                else if (testCombo(e._mod, KMOD_CTRL, KMOD_SHIFT))
                {
                    if (!hasSelection())
                        startSelection();

                    _cursorPos = !isSecret ? skipWord(false) : 0;
                    assert(_cursorPos <= _text.get().size());
                    invalidateContent();
                }
                actualizePrimarySelection();
                break;

            case SDLK_RIGHT:
                if (bool const hasShift = testMask(e._mod, KMOD_SHIFT);
                    hasShift || !e._mod)
                {
                    if (hasShift && !hasSelection())
                        startSelection();

                    if (!hasShift && hasSelection())
                    {
                        dropSelection();
                    }
                    else if (_cursorPos < _text.get().size())
                    {
                        _cursorPos += 1;
                        invalidateContent();
                    }
                }
                else if (testMask(e._mod, KMOD_CTRL) ||
                         testMask(e._mod, KMOD_ALT))
                {
                    _cursorPos = !isSecret ? skipWord(true)
                                           : _text.get().size();
                    assert(_cursorPos <= _text.get().size());
                    invalidateContent();
                }
                else if (testCombo(e._mod, KMOD_CTRL, KMOD_SHIFT))
                {
                    if (!hasSelection())
                        startSelection();

                    _cursorPos = !isSecret ? skipWord(true)
                                           : _text.get().size();
                    assert(_cursorPos <= _text.get().size());
                    invalidateContent();
                }
                actualizePrimarySelection();
                break;

            case SDLK_UP:
            case SDLK_PAGEUP:
            case SDLK_HOME:
                if (!e._mod)
                {
                    _cursorPos = 0;
                    dropSelection();
                    invalidateContent();
                }
                else if (testMask(e._mod, KMOD_SHIFT))
                {
                    if (!hasSelection())
                        startSelection();

                    _cursorPos = 0;
                    invalidateContent();
                }
                actualizePrimarySelection();
                break;

            case SDLK_DOWN:
            case SDLK_PAGEDOWN:
            case SDLK_END:
                if (!e._mod)
                {
                    _cursorPos = _text.get().size();
                    dropSelection();
                    invalidateContent();
                }
                else if (testMask(e._mod, KMOD_SHIFT))
                {
                    if (!hasSelection())
                        startSelection();

                    _cursorPos = _text.get().size();
                    invalidateContent();
                }
                actualizePrimarySelection();
                break;

            case SDLK_INSERT:
                if (!_enabled.get())
                    break;
                if (!e._mod)
                {
                    _insertMode = !(_insertMode.get());
                }
                else if (testMask(e._mod, KMOD_SHIFT))
                {
                    if (std::string const & primarySelection = overlayController().primarySelection();
                        !primarySelection.empty())
                    {
                        processInput(text::toUnicode(primarySelection));
                    }
                }
                break;

            case SDLK_a:
                if (testMask(e._mod, KMOD_CTRL) &&
                    !_text.get().empty())
                {
                    _selectionBegin = 0;
                    _cursorPos = _text.get().size();
                    actualizePrimarySelection();
                    invalidateContent();
                }
                break;

            case SDLK_c:
                if (!isSecret && testMask(e._mod, KMOD_CTRL))
                {
                    if (MaybeSelectionText const & selectionText = getSelectionText();
                        selectionText)
                    {
                        overlayController().setClipboardText(*selectionText);
                    }
                }
                break;

            case SDLK_x:
                if (testMask(e._mod, KMOD_CTRL))
                {
                    if (MaybeSelectionText const & selectionText = getSelectionText();
                        selectionText)
                    {
                        if (_enabled.get())
                        {
                            eraseSelection();
                        }

                        if (!isSecret)
                        {
                            overlayController().setClipboardText(*selectionText);
                        }
                    }
                }
                break;

            case SDLK_v:
                if (!_enabled.get())
                    break;
                if (testMask(e._mod, KMOD_CTRL))
                {
                    std::string const & clipboardText = overlayController().clipboardText();
                    processInput(text::toUnicode(clipboardText));
                }
                break;
        }
        CommonCallbacks::handle(e);
    }

    void Editbox::handle(application::OnTextInput const & e)
    {
        if (_enabled.get())
        {
            processInput(text::toUnicode(e._text));
        }
        CommonCallbacks::handle(e);
    }
}
