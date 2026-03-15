#pragma once

#include <minire/gui/theme.hpp>

#include <memory>

namespace minire::content { class Manager; }

namespace minire::gui_application
{
    // NOTE: Implementation will hold a lease to a builting atlas,
    //       therefore, the returned point must not outlive content::Manager.
    //       Otherwise, a fatal error will happen on Manger's dtor.
    std::unique_ptr<gui::Theme> makeDefaultTheme(content::Manager &);
}
