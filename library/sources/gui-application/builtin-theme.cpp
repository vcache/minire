#include <gui-application/builtin-theme.hpp>

#include <minire/content/manager.hpp>
#include <minire/errors.hpp>
#include <minire/formats/bdf.hpp>
#include <minire/formats/image.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/components/dropdown.hpp>
#include <minire/gui/components/editbox.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/components/listview.hpp>
#include <minire/gui/components/progress-bar.hpp>
#include <minire/gui/components/scrollbar.hpp>
#include <minire/gui/components/spinbox.hpp>
#include <minire/gui/components/text.hpp>
#include <minire/gui/theme.hpp>
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

namespace minire::gui_application
{
    static std::string const kTextureId = "__minire_builtin_atlas__";
    static std::string const kFontFaceId = "__minire_builtin_font_face__";
    static std::string const kFontFaceId6x13 = "__minire_builtin_font_face__6x13.bdf";
    static std::string const kFontFaceId6x13B = "__minire_builtin_font_face__6x13B.bdf";
    static std::string const kFontFaceId6x13O = "__minire_builtin_font_face__6x13O.bdf";

    namespace
    {
        models::sprite::Image mkImage(utils::Rect const & boundary,
                                      utils::Rect const & out,
                                      utils::Rect const & in)
        {
            return models::sprite::Image(
                kTextureId,
                utils::NinePatch
                {
                    ._boundary = boundary,
                    ._out = out,
                    ._in = in,
                });
        }

        models::sprite::Image mkImage(utils::Rect const & patch)
        {
            return models::sprite::Image(kTextureId, patch);
        }

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
            explicit BuiltinTheme(content::Manager & contentManager)
                : _atlas(contentManager.upload(
                    kTextureId,
                    formats::loadImage(kBuiltinGuiAtlas, kBuiltinGuiAtlas_len)))
                , _fontFace6x13(contentManager.upload(
                    kFontFaceId6x13,
                    loadBdf(kBuiltinGuiFontFace6x13, kBuiltinGuiFontFace6x13_len, "builtin:6x13.bdf")))
                , _fontFace6x13B(contentManager.upload(
                    kFontFaceId6x13B,
                    loadBdf(kBuiltinGuiFontFace6x13B, kBuiltinGuiFontFace6x13B_len, "builtin:6x13B.bdf")))
                , _fontFace6x13O(contentManager.upload(
                    kFontFaceId6x13O,
                    loadBdf(kBuiltinGuiFontFace6x13O, kBuiltinGuiFontFace6x13O_len, "builtin:6x13O.bdf")))
                , _fontFace(contentManager.upload(kFontFaceId, models::FontFace
                    {
                        ._regular = kFontFaceId6x13,
                        ._bold = kFontFaceId6x13B,
                        ._italic = kFontFaceId6x13O,
                        ._glyphWidth = 6,
                        ._glyphHeight = 13,
                    }))
        {}

        private:
            gui::Theme::Value const & getImpl(std::string const & component,
                                              std::string const & property,
                                              gui::Theme::Style const & style) const override
            {
                using namespace gui::components;

                static gui::Theme::Value const kIconArrowLeft(mkImage(utils::Rect(29, 1, 35, 11)));
                static gui::Theme::Value const kIconArrowUp(mkImage(utils::Rect(37, 1, 47, 11)));
                static gui::Theme::Value const kIconArrowRight(mkImage(utils::Rect(49, 1, 55, 11)));
                static gui::Theme::Value const kIconArrowDown(mkImage(utils::Rect(57, 1, 67, 11)));
                static gui::Theme::Value const kIconDecrease(mkImage(utils::Rect(69, 1, 78, 11)));
                static gui::Theme::Value const kIconIncrease(mkImage(utils::Rect(80, 1, 89, 11)));

                if ("i:arrow-left" == property)      return kIconArrowLeft;
                if ("i:arrow-up" == property)        return kIconArrowUp;
                if ("i:arrow-right" == property)     return kIconArrowRight;
                if ("i:arrow-down" == property)      return kIconArrowDown;
                if ("i:decrease" == property)        return kIconDecrease;
                if ("i:increase" == property)        return kIconIncrease;

                if (Button::kName == component)      return getButton(property, style);
                if (Dropdown::kName == component)    return getDropdown(property, style);
                if (Editbox::kName == component)     return getEdit(property, style);
                if (Image::kName == component)       { /* nothing */ }
                if (ListView::kName == component)    return getListview(property, style);
                if (ProgressBar::kName == component) return getProgressBar(property, style);
                if (Scrollbar::kName == component)   return getScrollbar(property, style);
                if (SpinBox::kName == component)     { /* nothing */ }
                if (Text::kName == component)        return getText(property, style);

                MINIRE_THROW("no such component");
            }

            gui::Theme::Value const & getButton(std::string const & property,
                                                gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kIconLocation(gui::Theme::Location::kLeft);
                static gui::Theme::Value const kIconSpacing(4.0f);
                static gui::Theme::Value const kContentPadding(utils::Rect(5));
                static gui::Theme::Value const kPressOffset(glm::vec2(2));
                static gui::Theme::Value const kBgNormal(mkImage(utils::Rect(0, 0, 12, 12),
                                                                 utils::Rect(2, 2, 10, 10),
                                                                 utils::Rect(5, 5, 7, 7)));
                static gui::Theme::Value const kBgHovered(mkImage(utils::Rect(0, 16, 12, 28),
                                                                  utils::Rect(2, 18, 10, 26),
                                                                  utils::Rect(5, 21, 7, 23)));
                static gui::Theme::Value const kBgPressed(mkImage(utils::Rect(0, 32, 12, 44),
                                                                  utils::Rect(2, 34, 10, 42),
                                                                  utils::Rect(5, 37, 7, 39)));

                if ("icon-location" == property)   return kIconLocation;
                if ("icon-spacing" == property)    return kIconSpacing;
                if ("content-padding" == property) return kContentPadding;
                if ("press-offset" == property)    return kPressOffset;
                if ("bg-normal" == property)       return kBgNormal;
                if ("bg-hovered" == property)      return kBgHovered;
                if ("bg-pressed" == property)      return kBgPressed;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & getScrollbar(std::string const & property,
                                                   gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kMinSliderLength(10.0f);
                static gui::Theme::Value const kBgPressed(mkImage(utils::Rect(0, 48, 13, 61),
                                                                  utils::Rect(2, 50, 11, 59),
                                                                  utils::Rect(5, 53, 8, 56)));

                if ("min-slider-length" == property) return kMinSliderLength;
                if ("bg" == property)                return kBgPressed;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & getListview(std::string const & property,
                                                  gui::Theme::Style const & style) const
            {
                static gui::Theme::Value const kPadding(utils::Rect(3));
                static gui::Theme::Value const kScrollbarWidth(21.0f);
                static gui::Theme::Value const kScrollbarAtLeft(false);
                static gui::Theme::Value const kBg(mkImage(utils::Rect(0, 65, 12, 77),
                                                           utils::Rect(2, 67, 10, 75),
                                                           utils::Rect(5, 70, 7, 72)));
                static gui::Theme::Value const kBgItemNormal(models::sprite::MaybeImage{});
                static gui::Theme::Value const kBgItemHovered(mkImage(utils::Rect(0, 97, 12, 109),
                                                                      utils::Rect(2, 99, 10, 107),
                                                                      utils::Rect(5, 102, 7, 104)));
                static gui::Theme::Value const kBgItemSelected(mkImage(utils::Rect(0, 113, 12, 125),
                                                                       utils::Rect(2, 115, 10, 123),
                                                                       utils::Rect(5, 118, 7, 120)));

                static gui::Theme::Value const kBg_Dropdown(mkImage(utils::Rect(0, 81, 12, 93),
                                                                    utils::Rect(2, 83, 10, 91),
                                                                    utils::Rect(5, 86, 7, 88)));

                // as a dropdown's tongue

                if (!style.empty() &&
                    gui::components::Dropdown::kName == style.back())
                {
                    if ("bg" == property) return kBg_Dropdown;
                }

                // default

                if ("padding" == property)           return kPadding;
                if ("scrollbar-width" == property)   return kScrollbarWidth;
                if ("scrollbar-at-left" == property) return kScrollbarAtLeft;
                if ("bg" == property)                return kBg;
                if ("bg-item-normal" == property)    return kBgItemNormal;
                if ("bg-item-hovered" == property)   return kBgItemHovered;
                if ("bg-item-selected" == property)  return kBgItemSelected;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & getDropdown(std::string const & property,
                                                  gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kPadding(utils::Rect(3));
                static gui::Theme::Value const kTongueMaxLines(5);
                static gui::Theme::Value const kTongueMinHeight(75.0f);
                static gui::Theme::Value const kTongueMaxHeight(250.0f);
                static gui::Theme::Value const kDropButtonWidth(21.0f);
                static gui::Theme::Value const kDropButtonAtLeft(false);
                static gui::Theme::Value const kBg(mkImage(utils::Rect(0, 65, 12, 77),
                                                           utils::Rect(2, 67, 10, 75),
                                                           utils::Rect(5, 70, 7, 72)));

                if ("padding" == property)             return kPadding;
                if ("t/max-lines" == property)         return kTongueMaxLines;
                if ("t/min-height" == property)        return kTongueMinHeight;
                if ("t/max-height" == property)        return kTongueMaxHeight;
                if ("drop-button-width" == property)   return kDropButtonWidth;
                if ("drop-button-at-left" == property) return kDropButtonAtLeft;
                if ("bg" == property)                  return kBg;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & getEdit(std::string const & property,
                                              gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kContentPadding(utils::Rect(5));
                static gui::Theme::Value const kActiveFormat(
                    text::Format().foreground(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))
                                  .background(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)));
                static gui::Theme::Value const kBgNormal(mkImage(utils::Rect(0, 65, 12, 77),
                                                                 utils::Rect(2, 67, 10, 75),
                                                                 utils::Rect(5, 70, 7, 72)));
                static gui::Theme::Value const kBgDisabled(mkImage(utils::Rect(0, 161, 12, 173),
                                                                   utils::Rect(2, 163, 10, 171),
                                                                   utils::Rect(5, 166, 7, 168)));
                static gui::Theme::Value const kCursorInsert(mkImage(utils::Rect(0, 129, 12, 141),
                                                                    utils::Rect(2, 131, 10, 139),
                                                                    utils::Rect(5, 134, 7, 136)));
                static gui::Theme::Value const kCursorReplace(mkImage(utils::Rect(0, 145, 12, 157),
                                                                      utils::Rect(2, 147, 10, 155),
                                                                      utils::Rect(5, 150, 7, 152)));
                static gui::Theme::Value const kSelectionBg(mkImage(utils::Rect(0, 113, 12, 125),
                                                                    utils::Rect(2, 115, 10, 123),
                                                                    utils::Rect(5, 118, 7, 120)));

                if ("content-padding" == property) return kContentPadding;
                if ("active-format" == property)   return kActiveFormat;
                if ("bg-normal" == property)       return kBgNormal;
                if ("bg-disabled" == property)     return kBgDisabled;
                if ("cursor-insert" == property)   return kCursorInsert;
                if ("cursor-replace" == property)  return kCursorReplace;
                if ("selection-bg" == property)    return kSelectionBg;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & getProgressBar(std::string const & property,
                                                     gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kSliderPadding(utils::Rect(3));
                static gui::Theme::Value const kBg(mkImage(utils::Rect(0, 161, 12, 173),
                                                           utils::Rect(2, 163, 10, 171),
                                                           utils::Rect(5, 166, 7, 168)));
                static gui::Theme::Value const kSlider(mkImage(utils::Rect(5, 118, 6, 119),
                                                               utils::Rect(5, 118, 6, 119),
                                                               utils::Rect(5, 118, 6, 119)));

                if ("slider-padding" == property) return kSliderPadding;
                if ("bg" == property)             return kBg;
                if ("slider" == property)         return kSlider;

                MINIRE_THROW("no such parameter");
            }

            gui::Theme::Value const & getText(std::string const & property,
                                              gui::Theme::Style const &) const
            {
                static gui::Theme::Value const kFontFace(kFontFaceId);

                if ("font-face" == property) return kFontFace;

                MINIRE_THROW("no such parameter");
            }

        private:
            std::unique_ptr<content::Lease> _atlas;
            std::unique_ptr<content::Lease> _fontFace6x13;
            std::unique_ptr<content::Lease> _fontFace6x13B;
            std::unique_ptr<content::Lease> _fontFace6x13O;
            std::unique_ptr<content::Lease> _fontFace;
        };
    }

    std::unique_ptr<gui::Theme> makeDefaultTheme(content::Manager & contentManager)
    {
        return std::make_unique<BuiltinTheme>(contentManager);
    }
}
