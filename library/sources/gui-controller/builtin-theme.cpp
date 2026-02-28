#include <minire/gui/theme.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/formats/bdf.hpp>
#include <minire/formats/image.hpp>
#include <minire/models/font-face.hpp>

#include <cassert>
#include <sstream>

extern unsigned int kBuiltinGuiAtlas_len;
extern unsigned char kBuiltinGuiAtlas[];

extern unsigned int kBuiltinGuiFontFace6x13_len;
extern unsigned char kBuiltinGuiFontFace6x13[];

extern unsigned int kBuiltinGuiFontFace6x13B_len;
extern unsigned char kBuiltinGuiFontFace6x13B[];

extern unsigned int kBuiltinGuiFontFace6x13O_len;
extern unsigned char kBuiltinGuiFontFace6x13O[];

namespace minire::gui_controller
{
    static std::string const kTextureId = "__minire_builtin_atlas__";
    static std::string const kFontFace = "__minire_builtin_font_face__";
    static std::string const kFontFace6x13 = "__minire_builtin_font_face__6x13.bdf";
    static std::string const kFontFace6x13B = "__minire_builtin_font_face__6x13B.bdf";
    static std::string const kFontFace6x13O = "__minire_builtin_font_face__6x13O.bdf";

    namespace
    {
        class BuiltinTheme
            : public gui::Theme
        {
            // TODO: move to std::ispanstream after migration to C++23,
            //       to avoid creation of temporary std::string
            static formats::Bdf::Sptr loadBdf(unsigned char const * data, size_t const size,
                                              std::string const & source)
            {
                std::string const contents(reinterpret_cast<char const *>(data), size);
                std::istringstream iss(contents);
                return std::make_shared<formats::Bdf>(iss, source);
            }

        public:
            explicit BuiltinTheme(content::Manager & contentManager,
                                  gui::ContentViewFactory & contentFactory)
                : _atlas(contentManager.upload(
                    kTextureId,
                    formats::loadImage(kBuiltinGuiAtlas, kBuiltinGuiAtlas_len)))
                , _fontFace6x13(contentManager.upload(
                    kFontFace6x13,
                    loadBdf(kBuiltinGuiFontFace6x13, kBuiltinGuiFontFace6x13_len, "builtin:6x13.bdf")))
                , _fontFace6x13B(contentManager.upload(
                    kFontFace6x13B,
                    loadBdf(kBuiltinGuiFontFace6x13B, kBuiltinGuiFontFace6x13B_len, "builtin:6x13B.bdf")))
                , _fontFace6x13O(contentManager.upload(
                    kFontFace6x13O,
                    loadBdf(kBuiltinGuiFontFace6x13O, kBuiltinGuiFontFace6x13O_len, "builtin:6x13O.bdf")))
                , _fontFace(contentManager.upload(kFontFace, models::FontFace
                    {
                        ._regular = kFontFace6x13,
                        ._bold = kFontFace6x13B,
                        ._italic = kFontFace6x13O,
                        ._glyphWidth = 6,
                        ._glyphHeight = 13,
                    }))
                , _contentFactory(contentFactory)
        {}

        private:

            // Images

            gui::ImageView::Sptr makeImageImpl(std::string const & component,
                                               std::string const & name,
                                               gui::Theme::Style const & style) const override
            {
                if ("i:arrow-left" == name)  return makeImageView(utils::Rect(29, 1, 35, 11));
                if ("i:arrow-up" == name)    return makeImageView(utils::Rect(37, 1, 47, 11));
                if ("i:arrow-right" == name) return makeImageView(utils::Rect(49, 1, 55, 11));
                if ("i:arrow-down" == name)  return makeImageView(utils::Rect(57, 1, 67, 11));
                if ("i:decrease" == name)    return makeImageView(utils::Rect(69, 1, 78, 11));
                if ("i:increase" == name)    return makeImageView(utils::Rect(80, 1, 89, 11));

                if ("button" == component)       return makeImageButton(name, style);
                if ("scrollbar" == component)    return makeImageScrollbar(name, style);
                if ("listview" == component)     return makeImageListview(name, style);
                if ("dropdown" == component)     return makeImageDropdown(name, style);
                if ("editbox" == component)      return makeImageEdit(name, style);
                if ("progress-bar" == component) return makeImageProgressBar(name, style);
                if ("spinbox" == component)      { /* nothing */ }

                MINIRE_THROW("unknown image or component");
            }

            gui::ImageView::Sptr makeImageButton(std::string const & name,
                                                 gui::Theme::Style const & /* style */) const
            {
                if ("bg-normal" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 0, 12, 12),
                        ._out = utils::Rect(2, 2, 10, 10),
                        ._in = utils::Rect(5, 5, 7, 7),
                    });

                if ("bg-hovered" == name)
                    return makeImageView(utils::NinePatch
                        {
                            ._boundary = utils::Rect(0, 16, 12, 28),
                            ._out = utils::Rect(2, 18, 10, 26),
                            ._in = utils::Rect(5, 21, 7, 23),
                        });

                if ("bg-pressed" == name)
                    return makeImageView(utils::NinePatch
                        {
                            ._boundary = utils::Rect(0, 32, 12, 44),
                            ._out = utils::Rect(2, 34, 10, 42),
                            ._in = utils::Rect(5, 37, 7, 39),
                        });

                MINIRE_THROW("unknown image");
            }

            gui::ImageView::Sptr makeImageScrollbar(std::string const & name,
                                                    gui::Theme::Style const & /* style */) const
            {
                if ("bg" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 48, 13, 61),
                        ._out = utils::Rect(2, 50, 11, 59),
                        ._in = utils::Rect(5, 53, 8, 56),
                    });

                MINIRE_THROW("unknown image");
            }

            gui::ImageView::Sptr makeImageListview(std::string const & name,
                                                   gui::Theme::Style const & style) const
            {
                // as a dropdown's tongue

                if ("dropdown" == style._modifier)
                {
                    if ("bg" == name)
                        return makeImageView(utils::NinePatch
                        {
                            ._boundary = utils::Rect(0, 81, 12, 93),
                            ._out = utils::Rect(2, 83, 10, 91),
                            ._in = utils::Rect(5, 86, 7, 88),
                        });
                }

                // default

                if ("bg" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 65, 12, 77),
                        ._out = utils::Rect(2, 67, 10, 75),
                        ._in = utils::Rect(5, 70, 7, 72),
                    });

                if ("bg-item-normal" == name)
                    return gui::ImageView::Sptr();

                if ("bg-item-hovered" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 97, 12, 109),
                        ._out = utils::Rect(2, 99, 10, 107),
                        ._in = utils::Rect(5, 102, 7, 104),
                    });

                if ("bg-item-selected" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 113, 12, 125),
                        ._out = utils::Rect(2, 115, 10, 123),
                        ._in = utils::Rect(5, 118, 7, 120),
                    });

                MINIRE_THROW("unknown image");
            }

            gui::ImageView::Sptr makeImageDropdown(std::string const & name,
                                                   gui::Theme::Style const & /* style */) const
            {
                if ("bg" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 65, 12, 77),
                        ._out = utils::Rect(2, 67, 10, 75),
                        ._in = utils::Rect(5, 70, 7, 72),
                    });

                MINIRE_THROW("unknown image");
            }

            gui::ImageView::Sptr makeImageEdit(std::string const & name,
                                               gui::Theme::Style const & /* style */) const
            {
                if ("bg-normal" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 65, 12, 77),
                        ._out = utils::Rect(2, 67, 10, 75),
                        ._in = utils::Rect(5, 70, 7, 72),
                    });

                if ("bg-disabled" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 161, 12, 173),
                        ._out = utils::Rect(2, 163, 10, 171),
                        ._in = utils::Rect(5, 166, 7, 168),
                    });

                if ("cursor-insert" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 129, 12, 141),
                        ._out = utils::Rect(2, 131, 10, 139),
                        ._in = utils::Rect(5, 134, 7, 136),
                    });

                if ("cursor-replace" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 145, 12, 157),
                        ._out = utils::Rect(2, 147, 10, 155),
                        ._in = utils::Rect(5, 150, 7, 152),
                    });

                if ("selection-bg" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 113, 12, 125),
                        ._out = utils::Rect(2, 115, 10, 123),
                        ._in = utils::Rect(5, 118, 7, 120),
                    });

                MINIRE_THROW("unknown image");
            }

            gui::ImageView::Sptr makeImageProgressBar(std::string const & name,
                                                      gui::Theme::Style const & /* style */) const
            {
                if ("bg" == name)
                    return makeImageView(utils::NinePatch
                    {
                        ._boundary = utils::Rect(0, 161, 12, 173),
                        ._out = utils::Rect(2, 163, 10, 171),
                        ._in = utils::Rect(5, 166, 7, 168),
                    });

                if ("slider" == name)
                    return makeImageView(utils::NinePatch
                    {

                        ._boundary = utils::Rect(5, 118, 6, 119),
                        ._out = utils::Rect(5, 118, 6, 119),
                        ._in = utils::Rect(5, 118, 6, 119),
                    });

                MINIRE_THROW("unknown image");
            }

        private:

            // Texts

            gui::TextView::Sptr makeTextImpl(std::string const & /*component*/,
                                             std::string const & /*name*/,
                                             gui::Theme::Style const & /*style*/,
                                             text::FormattedString const & string) const override
            {
                return _contentFactory.makeTextView(string, kFontFace);
            }

        private:

            // Paramters

            gui::Theme::Value const & parameterImpl(std::string const & component,
                                                    std::string const & name,
                                                    gui::Theme::Style const & style) const override
            {
                if ("button" == component)       return parameterButton(name, style);
                if ("scrollbar" == component)    return parameterScrollbar(name, style);
                if ("listview" == component)     return parameterListview(name, style);
                if ("dropdown" == component)     return parameterDropdown(name, style);
                if ("editbox" == component)      return parameterEdit(name, style);
                if ("progress-bar" == component) return parameterProgressBar(name, style);
                if ("spinbox" == component)      { /* nothing */ }

                MINIRE_THROW("no such component");
            }

            gui::Theme::Value const & parameterButton(std::string const & name,
                                                      gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kIconLocation(gui::Theme::Location::kLeft);
                static gui::Theme::Value const kIconSpacing(4.0f);
                static gui::Theme::Value const kPadding(utils::Rect(5));
                static gui::Theme::Value const kPressOffset(glm::vec2(2));

                if ("icon-location" == name) return kIconLocation;
                if ("icon-spacing" == name)  return kIconSpacing;
                if ("padding" == name)       return kPadding;
                if ("press-offset" == name)  return kPressOffset;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & parameterScrollbar(std::string const & name,
                                                         gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kMinSliderLength(10.0f);

                if ("min-slider-length" == name) return kMinSliderLength;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & parameterListview(std::string const & name,
                                                        gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kPadding(utils::Rect(3));
                static gui::Theme::Value const kScrollbarWidth(21.0f);
                static gui::Theme::Value const kScrollbarAtLeft(false);

                if ("padding" == name) return kPadding;
                if ("scrollbar-width" == name) return kScrollbarWidth;
                if ("scrollbar-at-left" == name) return kScrollbarAtLeft;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & parameterDropdown(std::string const & name,
                                                        gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kPadding(utils::Rect(3));
                static gui::Theme::Value const kTongueMaxLines(5);
                static gui::Theme::Value const kTongueMinHeight(75.0f);
                static gui::Theme::Value const kTongueMaxHeight(250.0f);
                static gui::Theme::Value const kDropButtonWidth(21.0f);
                static gui::Theme::Value const kDropButtonAtLeft(false);

                if ("padding" == name) return kPadding;
                if ("t/max-lines" == name) return kTongueMaxLines;
                if ("t/min-height" == name) return kTongueMinHeight;
                if ("t/max-height" == name) return kTongueMaxHeight;
                if ("drop-button-width" == name) return kDropButtonWidth;
                if ("drop-button-at-left" == name) return kDropButtonAtLeft;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & parameterEdit(std::string const & name,
                                                    gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kContentPadding(utils::Rect(5));
                static gui::Theme::Value const kActiveFormat(
                    text::Format().foreground(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
                                  .background(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)));

                if ("content-padding" == name) return kContentPadding;
                if ("active-format" == name) return kActiveFormat;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & parameterProgressBar(std::string const & name,
                                                           gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kSliderPadding(utils::Rect(3));

                if ("slider-padding" == name) return kSliderPadding;

                MINIRE_THROW("no such parameter");
            }

        private:
            gui::ImageView::Sptr makeImageView(utils::Rect const & rect) const
            {
                return _contentFactory.makeImageView(kTextureId, rect);
            }

            gui::ImageView::Sptr makeImageView(utils::NinePatch const & ninePatch) const
            {
                return _contentFactory.makeImageView(kTextureId, ninePatch);
            }

        private:
            std::unique_ptr<content::Lease> _atlas;
            std::unique_ptr<content::Lease> _fontFace6x13;
            std::unique_ptr<content::Lease> _fontFace6x13B;
            std::unique_ptr<content::Lease> _fontFace6x13O;
            std::unique_ptr<content::Lease> _fontFace;
            gui::ContentViewFactory &       _contentFactory;
        };
    }

    std::unique_ptr<gui::Theme> makeDefaultTheme(content::Manager & contentManager,
                                                 gui::ContentViewFactory & contentFactory)
    {
        return std::make_unique<BuiltinTheme>(contentManager, contentFactory);
    }
}
