#include <minire/gui/hot-keys.hpp>

#include <minire/errors.hpp>
#include <minire/logging.hpp>

#include <unordered_set>

namespace minire::gui
{
    void HotKeys::set(::SDL_Scancode key, uint16_t mods, Handler handler)
    {
        std::unordered_set<models::KeyCombo> inserted;
        auto candidates = models::KeyCombo::expandCombinedMods(key, mods);
        for(models::KeyCombo const & candidate : candidates)
        {
            if (inserted.contains(candidate))
                continue;

            auto [_, success] = _store.emplace(candidate, std::move(handler));
            MINIRE_INVARIANT(success, "hotkey is already set: {}", candidate);
            inserted.emplace(candidate);
            MINIRE_DEBUG("hotkey is registered: {}", candidate);
        }
    }

    bool HotKeys::handle(application::OnKeyDown const & e) const
    {
        if (auto it = _store.find(models::KeyCombo(e._code, e._mod));
            it != _store.cend())
        {
            if (it->second(e._code, e._mod))
            {
                MINIRE_DEBUG("fired hotkey for {}", it->first);
                return true;
            }
        }
        return false;
    }
}
