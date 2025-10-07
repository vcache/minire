#include <minire/gui/components/dropdown.hpp>

#include <utils/overloaded.hpp>

#include <minire/errors.hpp>
#include <minire/gui/layout.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/logging.hpp>
#include <minire/models/input-handler.hpp>

#include <fmt/format.h>

#include <cassert>

namespace minire::gui::components
{
    class Dropdown::TongueLayout
        : public Layout
    {
    public:
        explicit TongueLayout(Dropdown & dropdown,
                              size_t const rows)
            : _dropdown(dropdown)
            , _grid(rows, 1)
        {
            // NOTE: _grid doesn't need setParent() to be called,
            //       because notification will be done manually.
        }

        Area evaluate(Area const & client,
                      Component const & component) const override
        {
            auto scrollbar = _dropdown._tongue ? _dropdown._tongue->_scrollbar.lock()
                                               : Scrollbar::Sptr();

            if (scrollbar.get() == &component)
            {
                return Area
                {
                    ._left = client._left + client._width - _dropdown._scrollbarWidth,
                    ._top = client._top,
                    ._width = _dropdown._scrollbarWidth,
                    ._height = client._height,
                };
            }
            else
            {
                float const scrollbarWidth = scrollbar ? _dropdown._scrollbarWidth : 0;
                Area const itemsArea
                {
                    ._left = client._left,
                    ._top = client._top,
                    ._width = client._width - scrollbarWidth,
                    ._height = client._height,
                };
                return _grid.evaluate(itemsArea, component);
            }
        }

        void set(size_t row, std::string const & id)
        {
            _grid.set(row, 0, id);
        }

        void unset(std::string const & id)
        {
            _grid.unset(id);
        }

        void notify()
        {
            Layout::notify();
        }

    private:
        Dropdown    & _dropdown;
        layouts::Grid _grid;
    };

    // TODO: instead of this, onUnfocus + conditional hotKeys can be used
    class Dropdown::DefaultHandler
        : public minire::models::InputHandler
    {
    public:
        explicit DefaultHandler(Dropdown & dropdown)
            : _dropdown(dropdown)
        {}

        bool handle(events::application::OnMouseDown const &)  { exec(); return true; }
        bool handle(events::application::OnMouseWheel const & e)
        {
            _dropdown.wheelScroll(e._dy);
            return true;
        }

        // TODO: it will stop work if tongue would have a focus
        bool handle(events::application::OnKeyDown const & e)
        {
            switch(e._key)
            {
                case SDLK_ESCAPE:
                    exec();
                    break;

                case SDLK_UP:
                    _dropdown.wheelScroll(1);
                    break;

                case SDLK_DOWN:
                    _dropdown.wheelScroll(-1);
                    break;

                case SDLK_PAGEUP:
                    _dropdown.wheelScroll(_dropdown._tongueMaxLines);
                    break;

                case SDLK_PAGEDOWN:
                    _dropdown.wheelScroll(-_dropdown._tongueMaxLines);
                    break;

                case SDLK_HOME:
                    if (_dropdown._tongue)
                    {
                        _dropdown.wheelScroll(_dropdown._tongue->_offset + 1);
                    }
                    break;

                case SDLK_END:
                    if (_dropdown._tongue)
                    {
                        _dropdown.wheelScroll(-(_dropdown._contents.size() - _dropdown._tongue->_offset));
                    }
                    break;

                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    MINIRE_WARNING("selection from keyboard isn't yet supported :'(");
                    break;
            }
            return true;
        }

        void exec()
        {
            _dropdown.destroyOverlay();
        }

    private:
        Dropdown & _dropdown;
    };

    Dropdown::Dropdown(GuiController & controller,
                       std::string const & id,
                       std::shared_ptr<Container> const & parent)
        : Container(controller, id, parent)
    {}

    void Dropdown::init(Background const & baseBackground,
                        Background const & tongueBackground,
                        Button::Background const & buttonBackground,
                        Button::MaybeIcon const & buttonIcon,
                        Button::MaybeText const & buttonText,
                        Arrangers arrangers,
                        float tongueMaxHeight,
                        size_t tongueMaxLines,
                        std::optional<size_t> constantLineHeight)
    {
        MINIRE_INVARIANT(!_inited, "Dropdown cannot be initialized twice");

        _background = emplace<Image>("__bg__", baseBackground._texture,
            baseBackground._patch, Arrangers::fill());

        _tongueBackground = tongueBackground;

        _dropButton = emplace<Button>("__btn__", buttonBackground,
            buttonIcon, buttonText,
            Arrangers
            {
                ._horizontal = Arranger(position::More{},   dimension::Content{},
                                        _activeItemPaddings._left, _activeItemPaddings._right),
                ._vertical   = Arranger(position::Center{}, dimension::Content{},
                                        _activeItemPaddings._top, _activeItemPaddings._bottom),
            });
        _dropButton->setClickCallback([this](Button const &) { buildOverlay(); });

        _tongueMaxHeight = tongueMaxHeight;
        _tongueMaxLines = tongueMaxLines;
        _constantLineHeight = constantLineHeight;

        setArrangers(arrangers);

        _inited = true;
    }

    void Dropdown::buildOverlay()
    {
        assert(_inited);
        assert(!_tongue);

        _tongue = std::make_unique<Tongue>(Tongue
        {
            ._tag = fmt::format("__dropdown-{:#X}__",
                                reinterpret_cast<std::uintptr_t>(this)),
            ._defaultHandler = std::make_shared<DefaultHandler>(*this),
            ._container = {},
            ._scrollbar = {},
            ._offset = _selectedIndex ? *_selectedIndex : 0,
            ._subButtons = {},
        });

        gui::components::Container & overlay = guiPush(_tongue->_tag,
                                                       _tongue->_defaultHandler);

        _tongue->_container = overlay.emplace<Container>("__tongue__");
        _tongue->_container->emplace<Image>("__bg__", _tongueBackground._texture,
                                            _tongueBackground._patch, Arrangers::fill());
        refillOverlay(); // will also call rearrangeTongue();
    }

    void Dropdown::rearrangeTongue()
    {
        if (!_tongue) return;

        Area const & area = contentArea();
        assert(_tongue->_container);
        _tongue->_container->setArrangers(Arrangers
        {
            ._horizontal = Arranger(position::Constant{area._left},
                                    dimension::Constant{area._width}),
            ._vertical   = Arranger(position::Constant{area._top + area._height},
                                    dimension::Constant{_expectedTongueHeight > 0 ? _expectedTongueHeight
                                                                                  : _tongueMaxHeight}),
        });
    }

    void Dropdown::refillOverlay()
    {
        if (!_tongue) return;

        std::vector<Button::Sptr> items;
        items.reserve(_contents.size());

        // build item's components
        if (_itemBuilderCallback)
        {
            for(size_t i = 0; i < _contents.size(); ++i)
            {
                auto item = _itemBuilderCallback(_contents[i], i,
                                                 i == _selectedIndex,
                                                 Purpose::kTongueLine);
                if (item)
                {
                    items.push_back(item);
                }
            }
        }

        // calculate height
        float totalHeight = 0;
        if (_constantLineHeight)
        {
            totalHeight = (*_constantLineHeight) * items.size();
        }
        else
        {
            for(Button::Sptr const & item : items)
            {
                assert(item);

                Dimension verticalDimension = item->arrangers()._vertical.dimension();
                float const height = std::visit(utils::Overloaded
                {
                    [](dimension::Constant const & v) -> float { return v._dimension; },
                    [&item](dimension::Fraction const &) -> float
                    {
                        MINIRE_THROW("Dropdown's item cannot be measured by a Fraction: {}", item->id());
                    },
                    [](dimension::Fill const &) -> float { return -1; },
                    [&item](dimension::Content const &) -> float
                    {
                        std::optional<std::pair<float, float>> content = item->measureContent();
                        MINIRE_INVARIANT(content, "no measurable content: {}", item->id());
                        return content->second;
                    },
                }, verticalDimension);

                if (height < 0)
                {
                    totalHeight = 0;
                    break;
                }

                totalHeight += height;
            }
        }
        totalHeight = std::min(totalHeight, _tongueMaxHeight);
        _expectedTongueHeight = totalHeight;

        // build a container
        assert(_tongue->_container);
        Container & container = *_tongue->_container;
        container.erase("__items__");

        auto layout = std::make_shared<TongueLayout>(*this, _tongueMaxLines);
        auto itemsContainer = container.emplace<Container>("__items__", layout);
#if 0
    // TODO: implement this
        itemsContainer->setMouseWheelCallback(
            [this] (Image const &, events::application::OnMouseWheel const & e)
            {
                if (_tongue && _tongue->_defaultHandler)
                {
                    _tongue->_defaultHandler->handle(e);
                }
            });
#endif
        if (_scrollbarBuilderCallback)
        {
            auto scrollbar = _scrollbarBuilderCallback();
            _tongue->_scrollbar = scrollbar;
            if (scrollbar)
            {
                scrollbar->setArrangers(Arrangers::fill());
                size_t const steps = _contents.size() > _tongueMaxLines ? _contents.size() - _tongueMaxLines : 0;
                float const step = steps != 0 ? 1.0f / static_cast<float>(steps) : 1.0f;
                scrollbar->setStep(step);
                scrollbar->setValue(static_cast<float>(_tongue->_offset) * step);
                itemsContainer->emplace(scrollbar);
            }
        }

        itemsContainer->setArrangers(Arrangers
            {
                ._horizontal = Arranger(position::Center{}, dimension::Fill{},
                                        _tonguePaddings._left, _tonguePaddings._right),
                ._vertical   = Arranger(position::Center{}, dimension::Fill{},
                                        _tonguePaddings._top, _tonguePaddings._bottom),
            });

        if (size_t lastIndex = _tongue->_offset + _tongueMaxLines;
            lastIndex > _contents.size())
        {
            _tongue->_offset -= std::min(_tongue->_offset, lastIndex - _contents.size());
        }

        _tongue->_subButtons.clear();
        for(size_t i = 0; i < items.size(); ++i)
        {
            Button::Sptr const & subButton = items[i];

            MINIRE_INVARIANT(!subButton->hasClickCallback(),
                             "Dropdown's button can't have a custom click callback");
            subButton->setClickCallback([this, i](Button const &)
                {
                    select(i);
                    destroyOverlay();
                });

            MINIRE_INVARIANT(!subButton->hasMouseWheelCallback(),
                             "Dropdown's scrollbar can't have a custom mouse wheel callback");
            subButton->setMouseWheelCallback(
                [this] (Button const &, events::application::OnMouseWheel const & e)
                {
                    if (_tongue && _tongue->_defaultHandler)
                    {
                        _tongue->_defaultHandler->handle(e);
                    }
                });
            itemsContainer->emplace(subButton);
            _tongue->_subButtons.push_back(subButton);
            if (_tongue->_offset <= i && i < (_tongue->_offset + _tongueMaxLines))
            {
                layout->set(i - _tongue->_offset, subButton->id());
            }
            else
            {
                subButton->setVisible(false);
            }
        }
        layout->notify();

        if (auto scrollbar = _tongue->_scrollbar.lock(); scrollbar)
        {
            MINIRE_INVARIANT(!scrollbar->hasValueChangedCallback(),
                             "Dropdown's scrollbar can't have custom valueChange callback");

            scrollbar->setValueChangedCallback(
                [this, wlayout = std::weak_ptr<TongueLayout>(layout)]
                (Scrollbar const &, float, float value)
                {
                    auto layout = wlayout.lock();
                    if (!_tongue || !layout) return;

                    size_t const buttonsCnt = _tongue->_subButtons.size();
                    size_t const boundary = std::min(buttonsCnt, _tongueMaxLines);
                    size_t const newOffset =
                        value * static_cast<float>(buttonsCnt - boundary);
                    if (newOffset != _tongue->_offset)
                    {
                        for(size_t i = 0; i < boundary; ++i)
                        {
                            if (auto subButton = _tongue->_subButtons[_tongue->_offset + i].lock();
                                subButton)
                            {
                                layout->unset(subButton->id());
                                subButton->setVisible(false);
                            }
                        }

                        _tongue->_offset = newOffset;

                        for(size_t i = 0; i < boundary; ++i)
                        {
                            if (auto subButton = _tongue->_subButtons[_tongue->_offset + i].lock();
                                subButton)
                            {
                                layout->set(i, subButton->id());
                                subButton->setVisible(true);
                            }
                        }
                    }

                    layout->notify();
                });
        }

        rearrangeTongue();
    }

    void Dropdown::destroyOverlay()
    {
        assert(_tongue);                        // a tongue is created
        assert(_tongue->_tag == guiTopTag());   // the top overlay is our
        _tongue.reset();
        guiPop();
    }

    void Dropdown::onContentAreaChanged()
    {
        Container::onContentAreaChanged();
        rearrangeTongue();
    }

    void Dropdown::revalidateContents()
    {
        // ensure _selectedIndex sanity
        if (_selectedIndex &&
            *_selectedIndex >= _contents.size())
        {
            _selectedIndex.reset();
        }

        // actialize the active item
        if (auto activeItem = _activeItem.lock();
            activeItem)
        {
            erase(activeItem->id());
            _activeItem.reset();
        }

        if (_selectedIndex)
        {
            assert(*_selectedIndex < _contents.size());
            if (_itemBuilderCallback)
            {
                auto activeItem = _itemBuilderCallback(_contents[*_selectedIndex],
                                                       *_selectedIndex, true,
                                                       Purpose::kActiveLine);
                activeItem->setClickCallback([this](Button const &){ buildOverlay(); });

                auto arrangers = activeItem->arrangers();
                arrangers._horizontal.setMarginMin(_activeItemPaddings._left);
                arrangers._horizontal.setMarginMax(_activeItemPaddings._right);
                arrangers._vertical.setMarginMin(_activeItemPaddings._top);
                arrangers._vertical.setMarginMax(_activeItemPaddings._bottom);
                activeItem->setArrangers(arrangers);

                emplace(activeItem);
                if (_dropButton->zOrder() <= activeItem->zOrder())
                {
                    size_t tmp = _dropButton->zOrder();
                    _dropButton->setZOrder(activeItem->zOrder() + 1);
                    activeItem->setZOrder(tmp);
                }

                _activeItem = activeItem;
            }
        }

        // actialize the tongue
        refillOverlay();
    }

    void Dropdown::wheelScroll(int deltaY)
    {
        if (auto scrollbar = _tongue ? _tongue->_scrollbar.lock() : Scrollbar::Sptr();
            scrollbar)
        {
            float const delta = static_cast<float>(deltaY) * scrollbar->step();
            scrollbar->setValue(scrollbar->value() - delta);
        }
    }

    std::any const * Dropdown::selectedValue() const
    {
        if (!_selectedIndex)
            return nullptr;
        assert(*_selectedIndex < _contents.size());
        return &_contents.at(*_selectedIndex);
    }

    void Dropdown::select(std::optional<size_t> index)
    {
        if (_selectedIndex == index)
            return;

        auto previous = _selectedIndex;
        _selectedIndex = index;

        if (_selectionChangedCallback)
            _selectionChangedCallback(*this, previous, _selectedIndex);

        revalidateContents();
    }
}
