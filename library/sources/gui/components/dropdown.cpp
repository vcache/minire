#include <minire/gui/components/dropdown.hpp>

#include <minire/errors.hpp>
#include <minire/gui/components/scrollbar.hpp>
#include <minire/gui/layout.hpp>
#include <minire/gui/overlay-controller.hpp>
#include <minire/logging.hpp>
#include <minire/models/input-handler.hpp>

#include <glm/common.hpp> // for glm::clamp

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

namespace minire::gui::components
{
    static std::string const kActiveItemContainerId = "__baseItem__";
    static std::string const kBaseDropButtonId = "__dropButton__";

    // TODO: instead of this, onUnfocus + conditional hotKeys can be used
    class Dropdown::DefaultHandler
        : public minire::models::InputHandler
    {
    public:
        explicit DefaultHandler(Dropdown & dropdown)
            : _dropdown(dropdown)
        {}

        bool handle(minire::events::application::OnMouseUp const &)
        {
            _dropdown.closeTongue();
            return true;
        }

        bool handle(minire::events::application::OnMouseWheel const & e)
        {
            _dropdown.wheelScroll(e._dy);
            return true;
        }

        // TODO: it will stop work if tongue would have a focus
        bool handle(minire::events::application::OnKeyDown const & e)
        {
            switch(e._key)
            {
                case SDLK_ESCAPE:
                    _dropdown.closeTongue();
                    break;

                case SDLK_UP:
                    _dropdown.wheelScroll(1);
                    break;

                case SDLK_DOWN:
                    _dropdown.wheelScroll(-1);
                    break;

                case SDLK_PAGEUP:
                    if (_dropdown._tongueOverlay)
                    {
                        assert(_dropdown._tongueOverlay->_listview);
                        ListView & listview = *_dropdown._tongueOverlay->_listview;
                        listview.scrollPageUp();
                    }
                    break;

                case SDLK_PAGEDOWN:
                    if (_dropdown._tongueOverlay)
                    {
                        assert(_dropdown._tongueOverlay->_listview);
                        ListView & listview = *_dropdown._tongueOverlay->_listview;
                        listview.scrollPageDown();
                    }
                    break;

                case SDLK_HOME:
                    if (_dropdown._tongueOverlay)
                    {
                        assert(_dropdown._tongueOverlay->_listview);
                        ListView & listview = *_dropdown._tongueOverlay->_listview;
                        listview.scrollHome();
                    }
                    break;

                case SDLK_END:
                    if (_dropdown._tongueOverlay)
                    {
                        assert(_dropdown._tongueOverlay->_listview);
                        ListView & listview = *_dropdown._tongueOverlay->_listview;
                        listview.scrollEnd();
                    }
                    break;

                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    MINIRE_WARNING("selection from keyboard isn't yet supported :'(");
                    break;
            }
            return true;
        }

    private:
        Dropdown & _dropdown;
    };

    class Dropdown::ActiveItemContainer
        : public Component
    {
    public:
        using Component::Component;

        size_t revalidateContent(size_t zOffset,
                                 bool const effectiveVisible,
                                 Area const & contentArea,
                                 Area const & clippingWindow) override
        {
            if (_activeItem)
            {
                auto [size, isResizable] = _activeItem->measure();
                if (!isResizable && size.y < contentArea._height)
                {
                    Area itemContentArea
                    {
                        ._left = contentArea._left,
                        ._top = contentArea._top + (contentArea._height - size.y) * 0.5f,
                        ._width = contentArea._width,
                        ._height = contentArea._height,
                    };
                    _activeItem->setContentArea(itemContentArea);
                }
                else
                {
                    _activeItem->setContentArea(contentArea);
                }

                _activeItem->setClippingWindow(clippingWindow);
                _activeItem->setVisible(effectiveVisible);
                zOffset = _activeItem->onZOrderChanged(zOffset);
            }
            return zOffset;
        }

        void setActiveItem(ContentView::Sptr const & activeItem)
        {
            _activeItem = activeItem;
            if (_activeItem)
            {
                _activeItem->setContentInvalidator(shared_from_this());
            }
            invalidateContent();
        }

        bool hasActiveItem() const { return _activeItem.operator bool(); }

    private:
        ContentView::Sptr _activeItem;
    };

    Dropdown::Dropdown(std::string const & id,
                       Theme const & theme,
                       Theme::Style const & style,
                       OverlayController & overlayController,
                       ItemBuilderCallback baseItemBuilder,
                       ItemBuilderCallback tongueItemBuilder)
        : Component(id, theme, style, overlayController)
        , _background(*this, theme.makeImage("dropdown", "bg", style))
        , _tongue(*this, Tongue
            {
                ._maxLines = theme.parameter<size_t>("dropdown", "t/max-lines", style),
                ._minHeight = theme.parameter<float>("dropdown", "t/min-height", style),
                ._maxHeight = theme.parameter<float>("dropdown", "t/max-height", style),
            })
        , _contents(*this)
        , _lineHeight(*this, 0)
        , _activeItemContainer(std::make_shared<ActiveItemContainer>(
            kActiveItemContainerId, theme, style, overlayController))
        , _dropButton(std::make_shared<Button>(
            kBaseDropButtonId, theme, style, overlayController))
        , _dropdownLayout(std::make_shared<layouts::VerticalTool>(
            kActiveItemContainerId, kBaseDropButtonId,
            theme.parameter<float>("dropdown", "drop-button-width", style),
            theme.parameter<bool>("dropdown", "drop-button-at-left", style)))
        , _baseItemBuilderCallback(baseItemBuilder)
        , _tongueItemBuilderCallback(tongueItemBuilder)
    {
        layout() = _dropdownLayout;
        padding() = theme.parameter<utils::Rect>("dropdown", "padding", style);
    }

    Dropdown::~Dropdown()
    {
        closeTongue();
    }

    void Dropdown::initialize()
    {
        assert(_activeItemContainer);
        _activeItemContainer->setParent(shared_from_this());
        _activeItemContainer->setCallback(std::in_place_type<gui::events::OnClick>, "__open__",
            [this](Component const &, gui::events::OnClick const &)
            { openTongue(); });

        assert(_dropButton);
        _dropButton->setParent(shared_from_this());
        _dropButton->icon() = theme().makeImage("dropdown", "i:arrow-down", style());
        _dropButton->setCallback(std::in_place_type<gui::events::OnClick>, "__open__",
            [this](Component const &, gui::events::OnClick const &)
            { openTongue(); });
    }

    void Dropdown::handle(gui::events::OnClick const & e)
    {
        openTongue();
        Component::handle(e);
    }

    void Dropdown::openTongue()
    {
        if (_tongueOverlay)
            return;

        _tongueOverlay = std::make_unique<TongueOverlay>(TongueOverlay
            {
                ._tag = fmt::format("__dropdown-{:#X}__",
                                    reinterpret_cast<std::uintptr_t>(this)),
                ._defaultHandler = std::make_shared<DefaultHandler>(*this),
                ._listview = nullptr,
                ._destroy = false,
            });

        Component & overlay = overlayController().push(_tongueOverlay->_tag,
                                                       _tongueOverlay->_defaultHandler);
        _tongueOverlay->_listview = overlay.emplace<ListView>(
            Theme::Style{._name = style()._name, ._modifier = "dropdown"}, "__tongue__",
            _tongueItemBuilderCallback ? _tongueItemBuilderCallback
                                       : _baseItemBuilderCallback);

        *(_tongueOverlay->_listview->contents()) = _contents.get();
        _tongueOverlay->_listview->select(_selected);
        _tongueOverlay->_listview->lineHeight() = _lineHeight;
        _tongueOverlay->_listview->setCallback(
            std::in_place_type<listview::OnSelectionChanged>, "__dropdown__",
            [this] (Component const &, listview::OnSelectionChanged const & e)
            {
                select(e._current);
            });

        invalidate();
    }

    size_t Dropdown::revalidateContent(size_t zOffset,
                                       bool const effectiveVisible,
                                       Area const & contentArea,
                                       Area const & clippingWindow)
    {
        assert(_activeItemContainer);

        // maybe close the tongue
        if (_tongueOverlay && _tongueOverlay->_destroy)
        {
            closeTongue();
        }

        // perform consistency step
        if (_selected && *_selected >= _contents.get().size())
        {
            auto previous = _selected;
            _selected = std::nullopt;
            handle(dropdown::OnSelectionChanged{previous, _selected});

            _activeItemContainer->setActiveItem({});
        }

        // revalidate background (if any)
        if (auto background = _background.get(); background)
        {
            if (_background.isInvalidated())
            {
                background->setContentInvalidator(shared_from_this());
            }

            background->setContentArea(contentArea);
            background->setClippingWindow(clippingWindow);
            background->setVisible(effectiveVisible);
            zOffset = background->onZOrderChanged(zOffset);
        }

        // actualize active element
        if (_selected && (!_activeItemContainer->hasActiveItem() || _contents.isInvalidated()))
        {
            ContentView::Sptr activeItem;
            if (*_selected < _contents.get().size())
            {
                MINIRE_INVARIANT(_baseItemBuilderCallback, "no base item builder for \"{}\"", id());
                activeItem = _baseItemBuilderCallback(_contents.get().at(*_selected), *_selected);
            }
            _activeItemContainer->setActiveItem(activeItem);
        }

        // rearrange a tongue (if any)
        if (_tongueOverlay)
        {
            assert(_tongueOverlay->_listview);
            ListView & listview = *(_tongueOverlay->_listview);

            if (_contents.isInvalidated())
            {
                *(listview.contents()) = _contents.get();
            }

            Tongue const & tongue = _tongue.get();

            float const tongueTop = contentArea._top + contentArea._height;
            float const heightLimit = overlayController().topClientArea()._height - tongueTop;
            size_t const shownLines = std::min(_contents.get().size(),
                                               tongue._maxLines.value_or(0ULL));
            float const lineHeight = listview.lineHeight().get();

            float tongueHeight = static_cast<float>(shownLines) * lineHeight +
                                 listview.padding().get()._top +
                                 listview.padding().get()._bottom;
            tongueHeight = glm::clamp(tongueHeight,
                                      tongue._minHeight.value_or(.0f),
                                      std::min(tongue._maxHeight.value_or(heightLimit), heightLimit));

            listview.horizontal() = Arranger(position::Constant{contentArea._left},
                                             dimension::Constant{contentArea._width}),
            listview.vertical()   = Arranger(position::Constant{tongueTop},
                                             dimension::Constant{tongueHeight});

        }

        // finish
        _background.revalidate();
        _tongue.revalidate();
        _contents.revalidate();

        return zOffset;
    }


    void Dropdown::closeTongue()
    {
        if (_tongueOverlay)
        {
            if(_tongueOverlay->_tag == overlayController().topTag())
            {
                overlayController().pop();
            }
            _tongueOverlay.reset();
        }
    }

    void Dropdown::wheelScroll(int deltaY)
    {
        if (_tongueOverlay)
        {
            assert(_tongueOverlay->_listview);
            ListView & listview = *(_tongueOverlay->_listview);
            listview.scroll(deltaY);
        }
    }

    void Dropdown::select(Selected selected)
    {
        // close the tongue
        if (_tongueOverlay)
        {
            _tongueOverlay->_destroy = true;
            invalidate();
        }

        // ensure sanity
        if (selected && *selected >= _contents.get().size())
        {
            selected = std::nullopt;
        }

        // avoid non-chagning calls
        if (_selected == selected)
            return;

        // update the state
        Selected previous = _selected;
        _selected = selected;

        // actialize an active item
        assert(_activeItemContainer);
        _activeItemContainer->setActiveItem({});
        invalidateContent();

        // notify subscribers
        handle(dropdown::OnSelectionChanged{previous, _selected});
    }

    Dropdown::Selected const & Dropdown::selected() const { return _selected; }

    std::any const * Dropdown::current() const
    {
        if (!_selected)
        {
            return nullptr;
        }

        if (_contents.get().empty())
        {
            return nullptr;
        }

        size_t const index = std::min(_contents.get().size() - 1, *_selected);
        assert(index < _contents.get().size());
        return &(_contents.get().at(index));
    }
}
