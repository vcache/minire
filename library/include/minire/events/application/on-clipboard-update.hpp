#pragma once

#include <minire/events/application/base.hpp>

#include <string>

namespace minire::events::application
{
    struct OnClipboardUpdate : public Base
    {
        std::string _clipboardText; // utf-8
        std::string _primarySelection; // utf-8

        template<typename... BaseArgs>
        OnClipboardUpdate(std::string clipboardText,
                          std::string primarySelection,
                          BaseArgs && ... baseArgs)
            : Base(std::forward<BaseArgs>(baseArgs)...)
            , _clipboardText(std::move(clipboardText))
            , _primarySelection(std::move(primarySelection))
        {}
    };
}
