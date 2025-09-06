#pragma once

#include <minire/events/application/base.hpp>

#include <string>

namespace minire::events::application
{
    struct OnTextInput : public Base
    {
        std::string _text; // utf-8

        template<typename... BaseArgs>
        OnTextInput(std::string text,
                    BaseArgs && ... baseArgs)
            : Base(std::forward<BaseArgs>(baseArgs)...)
            , _text(std::move(text))
        {}
    };
}
