#pragma once

#include <minire/errors.hpp>
#include <minire/gui/content-view.hpp>
#include <minire/text/text-format.hpp>
#include <minire/utils/rect.hpp>

#include <glm/vec2.hpp>

#include <limits>
#include <memory>
#include <string>
#include <variant>

namespace minire::gui
{
    class Theme
    {
    public:
        enum class Location
        {
            kLeft, kTop, kRight, kBottom,
        };

        using Value = std::variant<std::monostate,
                                   bool,
                                   int64_t,
                                   float,
                                   std::string,
                                   glm::vec2,
                                   Location,
                                   utils::Rect,
                                   text::Format>;

        /**
         * \a _name - a primary name of a style (e.g. "builtin", "main-menu", "inventory", "hud", etc)
         * \a _modifier - an optional alternator of a style, for example:
         *                A ListView might be used as a separate component or as a part a Dropdown,
         *                and it might have different styles in both cases.
         * */
        struct Style
        {
            std::string _name;
            std::string _modifier;
        };

    public:
        virtual ~Theme() = default;

        /**
         * \a component - a type of component (e.g., "button", "scrollbar", "dropdown", etc)
         * \a name - how component is used, component-specific (e.g. "normal", "hovered", "pressed", etc)
         * \a style - stylistic variant of a (component, name) pair (e.g., "builtin", "menu", "hud" etc)
         * */
        template<typename T>
        auto parameter(std::string const & component,
                       std::string const & name,
                       Style const & style) const
        try
        {
            Value const & value = parameterImpl(component, name, style);
            if constexpr(std::is_same_v<T, size_t>)
            {
                int64_t result = std::get<int64_t>(value);
                MINIRE_INVARIANT(result >= 0, "cannot be converted to size_t: {}", result);
                return static_cast<size_t>(result);
            }
            else
            {
                return std::get<T>(value);
            }
        }
        catch(std::exception const &e)
        {
            MINIRE_THROW("failed to fetch a param (\"{}\", \"{}\", \"{}\"/\"{}\"): {}",
                         component, name, style._name, style._modifier, e.what());
        }
        catch(...)
        {
            MINIRE_THROW("failed to fetch a param (\"{}\", \"{}\", \"{}\"/\"{}\"): unknown exception",
                         component, name, style._name, style._modifier);
        }

        ImageView::Sptr makeImage(std::string const & component,
                                  std::string const & name,
                                  Style const & style) const
        try
        {
            return makeImageImpl(component, name, style);
        }
        catch(std::exception const &e)
        {
            MINIRE_THROW("failed to make an image (\"{}\", \"{}\", \"{}\"/\"{}\"): {}",
                         component, name, style._name, style._modifier, e.what());
        }
        catch(...)
        {
            MINIRE_THROW("failed to make an image (\"{}\", \"{}\", \"{}\"/\"{}\"): unknown exception",
                         component, name, style._name, style._modifier);
        }

        TextView::Sptr makeText(std::string const & component,
                                std::string const & name,
                                Style const & style,
                                text::FormattedString const & string) const
        try
        {
            return makeTextImpl(component, name, style, string);
        }
        catch(std::exception const &e)
        {
            MINIRE_THROW("failed to make a text (\"{}\", \"{}\", \"{}\"/\"{}\"): {}",
                         component, name, style._name, style._modifier, e.what());
        }
        catch(...)
        {
            MINIRE_THROW("failed to make a text (\"{}\", \"{}\", \"{}\"/\"{}\"): unknown exception",
                         component, name, style._name, style._modifier);
        }

    protected:
        virtual Value const & parameterImpl(std::string const & component,
                                            std::string const & name,
                                            Style const & style) const = 0;

        virtual ImageView::Sptr makeImageImpl(std::string const & component,
                                              std::string const & name,
                                              Style const & style) const = 0;

        virtual TextView::Sptr makeTextImpl(std::string const & component,
                                            std::string const & name,
                                            Style const & style,
                                            text::FormattedString const &) const = 0;
    };
}
