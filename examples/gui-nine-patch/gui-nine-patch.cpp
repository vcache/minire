#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/logging.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class GuiNinePatch
        : public minire::GuiApplication
    {
        using GuiApplication::GuiApplication;

    protected:
        void onStart() override
        {
            GuiApplication::onStart();

            auto ninePatch = guiRoot().emplace<minire::gui::components::Image>(
                "nine-patch-example",
                minire::models::sprite::Image(
                    "hud.png",
                    minire::utils::NinePatch
                    {
                        ._boundary = minire::utils::Rect(8, 8, 111, 111),
                        ._out = minire::utils::Rect(22, 22, 97, 97),
                        ._in = minire::utils::Rect(25, 25, 94, 94),
                    }));

            ninePatch->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                            minire::gui::dimension::Constant{100});
            ninePatch->vertical() = minire::gui::Arranger(minire::gui::position::Center{},
                                                          minire::gui::dimension::Constant{100});
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
        GuiNinePatch application(1280, 720, "GUI Nine Patch", manager);
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
