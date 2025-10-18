#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/logging.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class GuiNinePatch
        : public minire::GuiController
    {
        using GuiController::GuiController;

    protected:
        void start() override
        {
            GuiController::start();

            auto ninePatch = guiRoot().emplace<minire::gui::components::Image>(
                "nine-patch-example",
                makeImageView(
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
    static size_t const kMaxCtrlFps = 60;

    try
    {
        // Initialization
        minire::logging::setVerbosity(minire::logging::Level::kDebug);
        minire::content::Manager manager;
        manager.setReader<minire::content::readers::Filesystem>(MINIRE_EXAMPLE_PREFIX);
        minire::Application application(1280, 720, "GUI Nine Patch", manager);
        application.setController<GuiNinePatch>(kMaxCtrlFps);
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
