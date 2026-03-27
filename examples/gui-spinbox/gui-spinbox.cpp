#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/spinbox.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/logging.hpp>

#include <fmt/format.h>
#include <fmt/std.h>

#include <cstdlib> // for EXIT_SUCCESS

using namespace minire::gui;
using namespace minire::gui::components;
using namespace minire::utils;

namespace
{
    class GuiSpinbox
        : public minire::GuiApplication
    {
        using GuiApplication::GuiApplication;

    protected:
        void onStart() override
        {
            GuiApplication::onStart();

            auto layout = std::make_shared<layouts::Grid>(2, 1);
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = layout;
            container->horizontal() = Arranger::fill();
            container->vertical() = Arranger::fill();

            {
                auto spinbox = container->emplace<SpinBox>("spinedit-0");
                layout->set(0, 0, spinbox->id());
                spinbox->horizontal() = Arranger(position::Center{}, dimension::Constant{100});
                spinbox->vertical() = Arranger(position::Center{}, dimension::Constant{24});
                spinbox->setCallback(std::in_place_type<spinbox::OnValueChanged>, "foo",
                    [](Component & component, spinbox::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }

            {
                auto spinbox = container->emplace<SpinBox>("spinedit-1");
                layout->set(1, 0, spinbox->id());
                spinbox->horizontal() = Arranger(position::Center{}, dimension::Constant{150});
                spinbox->vertical() = Arranger(position::Center{}, dimension::Constant{24});
                spinbox->format() = "{:f}";
                spinbox->spacing() = 3;
                spinbox->setCallback(std::in_place_type<spinbox::OnValueChanged>, "foo",
                    [](Component & component, spinbox::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });

                spinbox->setValue(3.1415f);
            }
        }
    };
}

int main()
{
    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);
        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);
        GuiSpinbox application(1280, 720, "GUI Spinbox", manager);
        application.setGlDebug(false);
        application.setVsync(true);

        // Main loop
        application.run();

        // Finish
        return EXIT_SUCCESS;
    }
    catch(std::exception const & e)
    {
        MINIRE_ERROR("Fatal error:\n{}", e.what());
    }
    catch(...)
    {
        MINIRE_ERROR("Fatal error: (unknown error)");
    }

    return EXIT_FAILURE;
}
