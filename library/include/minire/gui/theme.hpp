#pragma once

#include <minire/content/id.hpp>
#include <minire/errors.hpp>
#include <minire/models/sprite.hpp>
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
                                   std::string, // also a content::Id
                                   glm::vec2,
                                   Location,
                                   utils::Rect,
                                   text::Format,
                                   models::sprite::MaybeImage>;

        /**
         * \a _name - a primary name of a style (e.g. "builtin", "main-menu", "inventory", "hud", etc)
         * \a _modifier - an optional alternator of a style, for example:
         *                A ListView might be used as a separate component or as a part a Dropdown,
         *                and it might have different styles in both cases.
         * */

        // TODO: instead "modifier", should use a std::vector<std::string>,
        //       so that deeply nested styles can be defined. For example:
        //          - ["dropdown", "listview"]
        //          - ["text"]
        //          - ["button", "text"]
        //          - ["scrollbar", "button", "text"]
        //          - ["dropdown", "button", "text"]

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
        // TODO: rename "parameter" -> "get"
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
            MINIRE_THROW("failed to fetch a param (\"{}\", \"{}\", \"{}\"/\"{}\"):\n{}",
                         component, name, style._name, style._modifier, e.what());
        }
        catch(...)
        {
            MINIRE_THROW("failed to fetch a param (\"{}\", \"{}\", \"{}\"/\"{}\"): unknown exception",
                         component, name, style._name, style._modifier);
        }

    protected:
        virtual Value const & parameterImpl(std::string const & component,
                                            std::string const & name,
                                            Style const & style) const = 0;
    };
}
