#include <minire/gui/theme.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/formats/image.hpp>

#include <cassert>

extern unsigned int kBuiltinGuiAtlas_len;
extern unsigned char kBuiltinGuiAtlas[];

namespace minire::gui_controller
{
    static std::string const kTextureId = "__minire_builtin_atlas__";

    namespace
    {
        class BuiltinButton
            : public gui::theme::Button
        {
        public:
            explicit BuiltinButton(gui::ContentViewFactory & contentFactory)
                : Button(Constants
                {
                    ._iconLocation = gui::theme::Location::kLeft,
                    ._iconSpacing = 4,
                    ._padding = utils::Rect(5),
                    ._pressOffset = glm::vec2(2),
                })
                , _contentFactory(contentFactory)
            {}

            gui::ImageView::Sptr makeNormalBg() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 0, 12, 12),
                        ._out = utils::Rect(2, 2, 10, 10),
                        ._in = utils::Rect(5, 5, 7, 7),
                    });
            }

            gui::ImageView::Sptr makeHoveredBg() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 16, 12, 28),
                        ._out = utils::Rect(2, 18, 10, 26),
                        ._in = utils::Rect(5, 21, 7, 23),
                    });
            }

            gui::ImageView::Sptr makePressedBg() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 32, 12, 44),
                        ._out = utils::Rect(2, 34, 10, 42),
                        ._in = utils::Rect(5, 37, 7, 39),
                    });
            }

        private:
            gui::ContentViewFactory & _contentFactory;
        };

        class BuiltinScrollbar
            : public gui::theme::Scrollbar
        {
        public:
            explicit BuiltinScrollbar(gui::ContentViewFactory & contentFactory)
                : Scrollbar(Constants
                {
                    ._minSliderLength = 10,
                })
                , _contentFactory(contentFactory)
            {}

            gui::ImageView::Sptr makeBackground() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 48, 13, 61),
                        ._out = utils::Rect(2, 50, 11, 59),
                        ._in = utils::Rect(5, 53, 8, 56),
                    });
            }

        private:
            gui::ContentViewFactory & _contentFactory;
        };

        class BuiltinListView
            : public gui::theme::ListView
        {
        public:
            explicit BuiltinListView(gui::ContentViewFactory & contentFactory)
                : ListView(Constants
                {
                    ._padding = utils::Rect(3),
                    ._scrollbarWidth = 21,
                    ._scrollbarAtLeft = false,
                })
                , _contentFactory(contentFactory)
            {}

            gui::ImageView::Sptr makeBackground() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 65, 12, 77),
                        ._out = utils::Rect(2, 67, 10, 75),
                        ._in = utils::Rect(5, 70, 7, 72),
                    });
            }

            gui::ImageView::Sptr makeNormalItemBackground() const override
            {
                return gui::ImageView::Sptr();
            }

            gui::ImageView::Sptr makeHoverItemBackground() const
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 97, 12, 109),
                        ._out = utils::Rect(2, 99, 10, 107),
                        ._in = utils::Rect(5, 102, 7, 104),
                    });
            }

            gui::ImageView::Sptr makeSelectedItemBackground() const
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 113, 12, 125),
                        ._out = utils::Rect(2, 115, 10, 123),
                        ._in = utils::Rect(5, 118, 7, 120),
                    });
            }

        protected:
            gui::ContentViewFactory & _contentFactory;
        };

        class BuiltinDropdown
            : public gui::theme::Dropdown
        {
            class DropdownListView
                : public BuiltinListView
            {
            public:
                using BuiltinListView::BuiltinListView;

                gui::ImageView::Sptr makeBackground() const override
                {
                    return _contentFactory.makeImageView(kTextureId,
                        utils::NinePatch
                        {
                            ._boundary = utils::Rect(0, 81, 12, 93),
                            ._out = utils::Rect(2, 83, 10, 91),
                            ._in = utils::Rect(5, 86, 7, 88),
                        });
                }
            };

        public:
            explicit BuiltinDropdown(gui::ContentViewFactory & contentFactory)
                : Dropdown(Constants
                {
                    ._padding = utils::Rect(3),
                    ._tongue = Constants::Tongue
                    {
                        ._maxLines = 5,
                        ._minHeight = 75.0f,
                        ._maxHeight = 250.0f,
                    },
                    ._dropButtonWidth = 21,
                    ._dropButtonAtLeft = false,
                })
                , _contentFactory(contentFactory)
                , _dropdownListView(std::make_shared<DropdownListView>(contentFactory))
            {}

            gui::ImageView::Sptr makeBackground() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 65, 12, 77),
                        ._out = utils::Rect(2, 67, 10, 75),
                        ._in = utils::Rect(5, 70, 7, 72),
                    });
            }

            gui::theme::ListView const & tongue() const override
            {
                assert(_dropdownListView);
                return *_dropdownListView;
            }

        private:
            gui::ContentViewFactory        & _contentFactory;
            std::shared_ptr<DropdownListView> _dropdownListView;
        };

        class BuiltinEditbox
            : public gui::theme::Editbox
        {
        public:
            explicit BuiltinEditbox(gui::ContentViewFactory & contentFactory)
                : Editbox(Constants
                {
                    ._contentPadding = utils::Rect(5),
                    ._activeFormat = text::Format().foreground(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
                                                   .background(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)),
                })
                , _contentFactory(contentFactory)
            {}

            gui::ImageView::Sptr makeNormalBg() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 65, 12, 77),
                        ._out = utils::Rect(2, 67, 10, 75),
                        ._in = utils::Rect(5, 70, 7, 72),
                    });
            }

            gui::ImageView::Sptr makeDisabledBg() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 161, 12, 173),
                        ._out = utils::Rect(2, 163, 10, 171),
                        ._in = utils::Rect(5, 166, 7, 168),
                    });
            }

            gui::ImageView::Sptr makeCursorImageInsert() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 129, 12, 141),
                        ._out = utils::Rect(2, 131, 10, 139),
                        ._in = utils::Rect(5, 134, 7, 136),
                    });
            }

            gui::ImageView::Sptr makeCursorImageReplace() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 145, 12, 157),
                        ._out = utils::Rect(2, 147, 10, 155),
                        ._in = utils::Rect(5, 150, 7, 152),
                    });
            }

            gui::ImageView::Sptr makeSelectionImage() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 113, 12, 125),
                        ._out = utils::Rect(2, 115, 10, 123),
                        ._in = utils::Rect(5, 118, 7, 120),
                    });
            }

        private:
            gui::ContentViewFactory & _contentFactory;
        };

        class BuiltinProgressBar
            : public gui::theme::ProgressBar
        {
        public:
            explicit BuiltinProgressBar(gui::ContentViewFactory & contentFactory)
                : ProgressBar(Constants
                {
                    ._sliderPadding = utils::Rect(3),
                })
                , _contentFactory(contentFactory)
            {}

            gui::ImageView::Sptr makeBackground() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 161, 12, 173),
                        ._out = utils::Rect(2, 163, 10, 171),
                        ._in = utils::Rect(5, 166, 7, 168),
                    });
            }

            gui::ImageView::Sptr makeSlider() const override
            {
                return _contentFactory.makeImageView(kTextureId,
                    utils::NinePatch
                    {

                        ._boundary = utils::Rect(5, 118, 6, 119),
                        ._out = utils::Rect(5, 118, 6, 119),
                        ._in = utils::Rect(5, 118, 6, 119),
                    });
            }

        private:
            gui::ContentViewFactory & _contentFactory;
        };

        class BuiltinTheme
            : public gui::Theme
        {
        public:
            explicit BuiltinTheme(content::Manager & contentManager,
                                   gui::ContentViewFactory & contentFactory)
                : _atlas(contentManager.upload(
                    kTextureId,
                    formats::loadImage(kBuiltinGuiAtlas, kBuiltinGuiAtlas_len)))
                , _contentFactory(contentFactory)
            {
                _button = std::make_shared<BuiltinButton>(_contentFactory);
                _scrollbar = std::make_shared<BuiltinScrollbar>(_contentFactory);
                _dropdown = std::make_shared<BuiltinDropdown>(_contentFactory);
                _listview = std::make_shared<BuiltinListView>(_contentFactory);
                _editbox = std::make_shared<BuiltinEditbox>(_contentFactory);
                _progressBar = std::make_shared<BuiltinProgressBar>(_contentFactory);
            }

            gui::ImageView::Sptr makeIcon(gui::theme::Icon icon) const override
            {
                switch(icon)
                {
                    case gui::theme::Icon::kArrowLeft:
                        return _contentFactory.makeImageView(kTextureId, utils::Rect(29, 1, 35, 11));

                    case gui::theme::Icon::kArrowUp:
                        return _contentFactory.makeImageView(kTextureId, utils::Rect(37, 1, 47, 11));

                    case gui::theme::Icon::kArrowRight:
                        return _contentFactory.makeImageView(kTextureId, utils::Rect(49, 1, 55, 11));

                    case gui::theme::Icon::kArrowDown:
                        return _contentFactory.makeImageView(kTextureId, utils::Rect(57, 1, 67, 11));

                    case gui::theme::Icon::kDecrease:
                        return _contentFactory.makeImageView(kTextureId, utils::Rect(69, 1, 78, 11));

                    case gui::theme::Icon::kIncrease:
                        return _contentFactory.makeImageView(kTextureId, utils::Rect(80, 1, 89, 11));
                }
                MINIRE_THROW("unknown Icon: {}", static_cast<int>(icon));
            }

            gui::theme::Button const & button() const override
            {
                assert(_button);
                return *_button;
            }

            gui::theme::Scrollbar const & scrollbar() const override
            {
                assert(_scrollbar);
                return *_scrollbar;
            }

            gui::theme::ListView const & listview() const override
            {
                assert(_listview);
                return *_listview;
            }

            gui::theme::Dropdown const & dropdown() const override
            {
                assert(_dropdown);
                return *_dropdown;
            }

            gui::theme::Editbox const & editbox() const override
            {
                assert(_editbox);
                return *_editbox;
            }

            gui::theme::ProgressBar const & progressBar() const override
            {
                assert(_progressBar);
                return *_progressBar;
            }

        private:
            std::unique_ptr<content::Lease>     _atlas;
            gui::ContentViewFactory &           _contentFactory;
            std::shared_ptr<BuiltinButton>      _button;
            std::shared_ptr<BuiltinScrollbar>   _scrollbar;
            std::shared_ptr<BuiltinListView>    _listview;
            std::shared_ptr<BuiltinDropdown>    _dropdown;
            std::shared_ptr<BuiltinEditbox>     _editbox;
            std::shared_ptr<BuiltinProgressBar> _progressBar;
        };
    }

    std::unique_ptr<gui::Theme> makeDefaultTheme(content::Manager & contentManager,
                                                  gui::ContentViewFactory & contentFactory)
    {
        return std::make_unique<BuiltinTheme>(contentManager, contentFactory);
    }
}
