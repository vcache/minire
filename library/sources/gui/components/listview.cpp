#include <minire/gui/components/listview.hpp>

#include <minire/errors.hpp>

#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

namespace minire::gui::components
{
    static std::string const kContentId = "__content__";
    static std::string const kScrollbarId = "__scrollbar__";

    class ListView::ListViewLayout
        : public LinearLayout
    {
    public:
        using Sptr = std::shared_ptr<ListViewLayout>;

        Area evaluate(Area const & clientArea,
                      Component const & component) const override
        {
            if (_dirty)
            {
                float top = 0;
                for(Row::Sptr const & row : _ordering)
                {
                    assert(row);
                    row->_top = top;
                    top += row->_height;
                }
                _dirty = false;
            }

            auto it = _store.find(component.id());
            MINIRE_INVARIANT(it != _store.end(), "unknown ListView component: \"{}\"",
                             component.id());
            Row::Sptr const & row = it->second;
            assert(row);

            return Area
            {
                ._left = clientArea._left,
                ._top = clientArea._top + row->_top,
                ._width = clientArea._width,
                ._height = row->_height,
            };
        }

        void onErase(Component const & component) override
        {
            MINIRE_THROW("onErase isn't supported: \"{}\"", component.id());
        }

        void onClear() override
        {
            _store.clear();
            _ordering.clear();
            _dirty = true;
        }

        void pushBack(std::string const & componentId,
                      float height)
        {
            Row::Sptr row = std::make_shared<Row>(
                Row
                {
                    ._componentId = componentId,
                    ._height = height,
                    ._top = 0,
                });

            auto [_, inserted] = _store.emplace(componentId, row);
            MINIRE_INVARIANT(inserted, "failed to inserted component: \"{}\"",
                             componentId);
            _ordering.push_back(row);
            _dirty = true;
        }

    private:
        struct Row
        {
            std::string _componentId;
            float       _height;
            float       _top;

            using Sptr = std::shared_ptr<Row>;
            using Wptr = std::weak_ptr<Row>;
        };

        using Ordering = std::vector<Row::Sptr>;
        using Store = std::unordered_map<std::string, Row::Sptr>;

        Store        _store;
        Ordering     _ordering;
        mutable bool _dirty = true;
    };

    namespace
    {
        class ListViewItem
            : public Component
        {
        public:
            ListViewItem(std::string const & id,
                         Theme const & theme,
                         Theme::Style const & style,
                         OverlayController & overlayController,
                         Component::Sptr const & contents,
                         ListView & listview,
                         size_t index)
                : Component(id, theme, style, overlayController)
                , _normalBackground(*this, theme.makeImage("listview", "bg-item-normal", style))
                , _hoverBackground(*this, theme.makeImage("listview", "bg-item-hovered", style))
                , _selectedBackground(*this, theme.makeImage("listview", "bg-item-selected", style))
                , _isSelected(*this, false)
                , _contents(contents)
                , _listview(listview)
                , _index(index)
            {}

            void handle(gui::events::OnMouseEnter const & e) override
            {
                invalidateContent();
                Component::handle(e);
            }

            void handle(gui::events::OnMouseLeave const & e) override
            {
                if (auto const & ptr = _hoverBackground.get(); ptr)
                {
                    ptr->setVisible(false);
                }
                Component::handle(e);
            }

            void handle(minire::events::application::OnMouseWheel const & e) override
            {
                _listview.scroll(e._dy);
                Component::handle(e);
            }

            void handle(gui::events::OnClick const &) override
            {
                _listview.select(_index);
            }

            size_t index() const { return _index; }

            Property<bool> const & isSelected() const { return _isSelected; }

            Property<bool> & isSelected() { return _isSelected; }

        protected:
            void initialize() override
            {
                auto sharedFromThis = shared_from_this();

                if (auto const & ptr = _normalBackground.get(); ptr)
                {
                    ptr->setContentInvalidator(sharedFromThis);
                    ptr->setVisible(!_isSelected.get() && isHovered());
                }

                if (auto const & ptr = _hoverBackground.get(); ptr)
                {
                    ptr->setContentInvalidator(sharedFromThis);
                    ptr->setVisible(!_isSelected.get() && isHovered());
                }

                if (auto const & ptr = _selectedBackground.get(); ptr)
                {
                    ptr->setContentInvalidator(sharedFromThis);
                    ptr->setVisible(_isSelected.get());
                }

                if (_contents)
                {
                    _contents->setParent(sharedFromThis);
                    _contents->setCallback(std::in_place_type<gui::events::OnMouseEnter>, "__enter__",
                        [this](Component const &, gui::events::OnMouseEnter const & e)
                        {
                            handle(e);
                        });
                    _contents->setCallback(std::in_place_type<gui::events::OnMouseLeave>, "__leave__",
                        [this](Component const &, gui::events::OnMouseLeave const & e)
                        {
                            handle(e);
                        });
                    _contents->setCallback(std::in_place_type<gui::events::OnClick>, "__cleack__",
                        [this](Component const &, gui::events::OnClick const & e)
                        {
                            handle(e);
                        });
                    _contents->setCallback(std::in_place_type<minire::events::application::OnMouseWheel>, "__wheel__",
                        [this](Component const &, minire::events::application::OnMouseWheel const & e)
                        {
                            handle(e);
                        });
                }
            }

            size_t revalidateContent(size_t zOffset,
                                     bool const effectiveVisible,
                                     Area const & contentArea,
                                     Area const & clippingWindow) override
            {
                auto sharedFromThis = shared_from_this();

                bool const isReallyHovered = isHovered() ||
                    (_contents && _contents->isHovered());

                if (auto const & ptr = _normalBackground.get(); ptr)
                {
                    if (_normalBackground.isInvalidated())
                    {
                        ptr->setContentInvalidator(sharedFromThis);
                    }

                    ptr->setContentArea(contentArea);
                    ptr->setClippingWindow(clippingWindow);
                    ptr->setVisible(!_isSelected.get() && !isReallyHovered &&
                                    effectiveVisible);
                    zOffset = ptr->onZOrderChanged(zOffset);
                }

                if (auto const & ptr = _hoverBackground.get(); ptr)
                {
                    if (_hoverBackground.isInvalidated())
                    {
                        ptr->setContentInvalidator(sharedFromThis);
                    }

                    ptr->setContentArea(contentArea);
                    ptr->setClippingWindow(clippingWindow);
                    ptr->setVisible(!_isSelected.get() && isReallyHovered &&
                                    effectiveVisible);
                    zOffset = ptr->onZOrderChanged(zOffset);
                }

                if (auto const & ptr = _selectedBackground.get(); ptr)
                {
                    if (_selectedBackground.isInvalidated())
                    {
                        ptr->setContentInvalidator(sharedFromThis);
                    }

                    ptr->setContentArea(contentArea);
                    ptr->setClippingWindow(clippingWindow);
                    ptr->setVisible(_isSelected.get() && effectiveVisible);
                    zOffset = ptr->onZOrderChanged(zOffset);
                }

                _normalBackground.revalidate();
                _hoverBackground.revalidate();
                _selectedBackground.revalidate();
                _isSelected.revalidate();

                return zOffset;
            }

            std::optional<std::pair<float, float>> measureContent() const override
            {
                if (!_contents) return std::nullopt;
                return _contents->measureContent();
            }

        private:
            Property<ImageView::Sptr> _normalBackground;
            Property<ImageView::Sptr> _hoverBackground;
            Property<ImageView::Sptr> _selectedBackground;
            Property<bool>            _isSelected;

            Component::Sptr           _contents;

            ListView &                _listview;
            size_t const              _index;
        };
    }

    ListView::ListView(std::string const & id,
                       Theme const & theme,
                       Theme::Style const & style,
                       OverlayController & overlayController,
                       ItemBuilderCallback itemBuilderCallback)
        : Component(id, theme, style, overlayController)
        , _background(*this, theme.makeImage("listview", "bg", style))
        , _contents(*this)
        , _offset(*this, 0)
        , _lineHeight(*this, 0)
        , _scrollbar(std::make_shared<Scrollbar>("__scrollbar__", theme, style, overlayController, true))
        , _contentLayout(std::make_shared<ListViewLayout>())
        , _verticalToolLayout(std::make_shared<layouts::VerticalTool>(
            kContentId, kScrollbarId,
            theme.parameter<float>("listview", "scrollbar-width", style),
            theme.parameter<bool>("listview", "scrollbar-at-left", style)))
        , _itemBuilderCallback(itemBuilderCallback)
    {
        layout() = _verticalToolLayout;
        padding() = theme.parameter<utils::Rect>("listview", "padding", style);
    }

    void ListView::initialize()
    {
        auto sharedFromThis = shared_from_this();

        if (_scrollbar)
        {
            _scrollbar->setParent(sharedFromThis);
            _scrollbar->step() = 1.0;
            _scrollbar->setValue(0);
            _scrollbar->setCallback(std::in_place_type<scrollbar::OnValueChanged>, "__scroll__",
                [this](Component &, scrollbar::OnValueChanged const & e)
                {
                    size_t const totalLines = _contents.get().size();
                    float const amplitude = totalLines > _showLines
                        ? static_cast<float>(totalLines - _showLines) : .0f;
                    _offset = static_cast<size_t>(e._current * amplitude);
                });

            _contentContainer = emplace<Component>(kContentId);
            _contentContainer->layout() = _contentLayout;
        }
    }

    size_t ListView::revalidateContent(size_t zOffset,
                                       bool const effectiveVisible,
                                       Area const & contentArea,
                                       Area const & clippingWindow)
    {
        if (_selected && *_selected >= _contents.get().size())
        {
            auto previous = _selected;
            _selected = std::nullopt;
            handle(listview::OnSelectionChanged{previous, _selected});
        }

        if (auto const & background = _background.get(); background)
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

        float const contentPadding = _contentContainer
            ? _contentContainer->padding().get()._top + _contentContainer->padding().get()._bottom
            : 0;
        float const heightLimit = contentArea._height - contentPadding;

        if (_contents.isInvalidated() || _offset.isInvalidated() ||
            _lineHeight.isInvalidated() || _heightLimit != heightLimit)
        {
            _contentContainer->clear();

            MINIRE_INVARIANT(_itemBuilderCallback,
                             "item builder callback isn't set: \"{}\"", id());
            float totalHeight = 0;
            size_t const totalLines = _contents.get().size();
            for (size_t index = _offset.get();
                index < totalLines && totalHeight < heightLimit;
                ++index)
            {
                if (auto const & item = _itemBuilderCallback(_contents.get().at(index), index,
                                                             theme(), style(), overlayController());
                    item)
                {
                    if (_lineHeight.get() <= 0)
                    {
                        auto const maybeSize = item->measureContent();
                        MINIRE_INVARIANT(maybeSize, "ListView's item must be measurable, or "
                                                    "ListView::lineHeight() must be set");
                        _lineHeight = maybeSize->second;
                    }
                    auto listViewItem = _contentContainer->emplace<ListViewItem>(
                        fmt::format("__item/{}__", index), item, *this, index);
                    listViewItem->isSelected() = (_selected && *_selected == index);
                    _contentLayout->pushBack(listViewItem->id(), _lineHeight.get());
                    totalHeight += _lineHeight.get();
                }
            }

            if (_scrollbar)
            {
                _showLines = static_cast<size_t>(std::floor(heightLimit / _lineHeight.get()));
                _scrollbar->step() = totalLines > _showLines
                    ? 1.0f / static_cast<float>(totalLines - _showLines) : 1.0f;
                if (_scrollbar->step().isInvalidated())
                {
                    if (_hasIntialScrollDone)
                    {
                        _scrollbar->setValue(static_cast<float>(_offset.get()) * _scrollbar->step().get());
                    }
                    else
                    {
                        scrollToSelected();
                        _hasIntialScrollDone = true;
                        // NOTE: this dirty hack is required because scrollToSelected()
                        //       will recalc _offset, thus, ListViewElements should be
                        //       re-created.
                        return revalidateContent(zOffset, effectiveVisible, contentArea, clippingWindow);
                    }
                }
            }

            _heightLimit = heightLimit;
        }

        _background.revalidate();
        _contents.revalidate();
        _offset.revalidate();
        _lineHeight.revalidate();

        return zOffset;
    }

    void ListView::select(Selected selected)
    {
        if (selected && *selected >= _contents.get().size())
        {
            selected = std::nullopt;
        }

        if (_selected == selected)
            return;

        Selected previous = _selected;
        _selected = selected;

        if (_contentContainer)
        {
            if (previous)
            {
                std::string const id = fmt::format("__item/{}__", *previous);
                if (auto const & previousSelected = _contentContainer->find<ListViewItem>(id);
                    previousSelected && previousSelected->index() == *previous)
                {
                    previousSelected->isSelected() = false;
                }
            }

            if (_selected)
            {
                std::string const id = fmt::format("__item/{}__", *_selected);
                if (auto const & currentlySelected = _contentContainer->find<ListViewItem>(id);
                    currentlySelected && currentlySelected->index() == *_selected)
                {
                    currentlySelected->isSelected() = true;
                }
                scrollToSelected();
            }
        }

        handle(listview::OnSelectionChanged{previous, _selected});
    }

    ListView::Selected const & ListView::selected() const
    {
        return _selected;
    }

    std::any const * ListView::current() const
    {
        return _selected ? &(_contents.get().at(*_selected)) : nullptr;
    }

    void ListView::scroll(int deltaY)
    {
        if (_scrollbar)
        {
            float const delta = static_cast<float>(deltaY) * _scrollbar->step().get();
            _scrollbar->setValue(_scrollbar->value() - delta);
        }
    }

    void ListView::scrollPageUp()
    {
        scroll(_showLines);
    }

    void ListView::scrollPageDown()
    {
        MINIRE_INVARIANT(std::numeric_limits<int>::max() >= _showLines,
                         "too many lines: {} < {}", std::numeric_limits<int>::max(),
                         _showLines);
        scroll(-static_cast<int>(_showLines));
    }

    void ListView::scrollHome()
    {
        _scrollbar->setValue(0.0f);
    }

    void ListView::scrollEnd()
    {
        _scrollbar->setValue(1.0f);
    }

    void ListView::scrollToSelected()
    {
        if (_scrollbar && _selected &&
            (*_selected < _offset.get() || *_selected > _offset.get() + _showLines))
        {
            _scrollbar->setValue(static_cast<float>(*_selected) * _scrollbar->step().get());
        }
    }
}
