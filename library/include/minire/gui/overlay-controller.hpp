#pragma once

#include <minire/application/input-handler.hpp>
#include <minire/content/id.hpp>
#include <minire/gui/area.hpp>
#include <minire/label.hpp>
#include <minire/models/system-cursor.hpp>
#include <minire/sprite.hpp>

#include <glm/vec2.hpp>

#include <memory>
#include <string>
#include <utility>
#include <variant>

namespace minire::text { class FormattedString; }
namespace minire::utils { class TextLayout; }

namespace minire::gui
{
    class Component;

    namespace overlay_input_mode
    {
        struct Active
        {
            application::InputHandler::Wptr _fallbackHandler;
        };

        struct Transparent {};
    }

    using OverlayInputMode = std::variant<overlay_input_mode::Active,
                                          overlay_input_mode::Transparent>;

    // TODO: rename to somethinng more appropriate.

    //       The aim of this class is to provide a limited interface
    //       over the GuiApplication, so that GUI components can safely use it.
    class OverlayController
    {
    public:
        virtual ~OverlayController() = default;

        virtual Component & push(std::string const & tag,
                                 OverlayInputMode const & = overlay_input_mode::Active{}) = 0;

        virtual std::string const & topTag() const = 0;

        virtual Area const & topClientArea() const = 0;

        virtual void pop() = 0;

    public:
        virtual void startTextInput() = 0;
        virtual void stopTextInput() = 0;

        virtual void setClipboardText(std::string const &) = 0;
        virtual void setPrimarySelection(std::string const &) = 0;

        virtual std::string clipboardText() const = 0;
        virtual std::string primarySelection() const = 0;

        virtual void setSystemCursor(minire::models::SystemCursor) = 0;

        virtual void unfocus() = 0;

    public:
        virtual Sprite::Sptr make(minire::models::Sprite) = 0;
        virtual Label::Sptr make(minire::models::Label) = 0;

        // returns (min size, is resizeable)
        virtual std::pair<glm::vec2, bool> measure(minire::models::sprite::Image const &) const = 0;

        virtual glm::vec2 measure(text::FormattedString const &,
                                  content::Id const &) const = 0;

        virtual std::unique_ptr<utils::TextLayout> layout(text::FormattedString const &,
                                                          content::Id const &) const = 0;
    };
}
