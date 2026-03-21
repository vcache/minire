#pragma once

#include <minire/application/events.hpp>
#include <minire/models/key-combo.hpp>

#include <functional>
#include <unordered_map>

namespace minire::gui
{
    class HotKeys
    {
    public:
        using Handler = std::function<bool(::SDL_Scancode, uint16_t)>;

        void set(::SDL_Scancode key, uint16_t mods, Handler handler);

    public:
        bool handle(application::OnKeyDown const & e) const;

    private:
        std::unordered_map<minire::models::KeyCombo, Handler> _store;
    };
}
