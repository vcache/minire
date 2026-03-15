#pragma once

#include <minire/content/id.hpp>
#include <minire/errors.hpp>
#include <minire/models/sprite.hpp>
#include <minire/text/text-format.hpp>
#include <minire/utils/rect.hpp>

#include <fmt/ranges.h>
#include <glm/vec2.hpp>

#include <limits>
#include <memory>
#include <string>
#include <variant>
#include <vector>

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
         * Style defines alternation for a given (componen, property) pair. Styles may be cascaded,
         * so that, the properties are inherited from common contexts and are overriden by more
         * specific contexts. The last style has the most priority.
         * 
         * For example,
         *  - component="text", property="font-face" and style could be:
         *    - []                                  - just a default text
         *    - ["button"]                          - text on a button component
         *    - ["listview"]                        - text on a listview component
         *    - ["dropdown", "listview"]            - text on a listview component of a dropdown's tongue
         *    - ["dropdown", "listview", "button"]  - text on a button of a listview component of a dropdown's tongue
         * 
         * Or for example: ["dropdown", "listview", "scrollbar"] - a scrollbar of a listview of a dropdown.
         * */
        using Style = std::vector<std::string>;

    public:
        virtual ~Theme() = default;

        /**
         * \a component - a type of component (e.g., "button", "scrollbar", "dropdown", etc)
         * \a property - how component is used, component-specific (e.g. "normal", "hovered", "pressed", etc)
         * \a style - stylistic variant of a (component, property ) pair (e.g., "builtin", "menu", "hud" etc)
         * */
        template<typename T>
        auto get(std::string const & component,
                 std::string const & property,
                 Style const & style) const
        try
        {
            Value const & value = getImpl(component, property, style);
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
            MINIRE_THROW("failed to fetch a property \"{}\" of \"{}\" with following style: {}:\n{}",
                         property, component, style, e.what());
        }
        catch(...)
        {
            MINIRE_THROW("failed to fetch a property \"{}\" of \"{}\" with following style: {}: unknown exception",
                         property, component, style);
        }

    protected:
        virtual Value const & getImpl(std::string const & component,
                                      std::string const & property,
                                      Style const & style) const = 0;
    };

    // Helpers

    inline Theme::Style concat(Theme::Style const & lhs,
                               std::string rhs)
    {
        Theme::Style result = lhs;
        result.emplace_back(std::move(rhs));
        return result;
    }
}
