#pragma once

#include <glm/gtx/string_cast.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <fmt/format.h>

namespace minire::logging
{
    template<typename T>
    constexpr bool kIsGlmFormattable = std::is_same_v<T, ::glm::vec2>
                                    || std::is_same_v<T, ::glm::vec3>
                                    || std::is_same_v<T, ::glm::vec4>
                                    || std::is_same_v<T, ::glm::mat4>;
}

template <typename T>
struct fmt::formatter<T, std::enable_if_t<minire::logging::kIsGlmFormattable<T>, char>>
    : fmt::formatter<std::string>
{
    template <typename FormatCtx>
    auto format(T const & value, FormatCtx & ctx) const
    {
        return fmt::formatter<std::string>::format(glm::to_string(value), ctx);
    }
};
