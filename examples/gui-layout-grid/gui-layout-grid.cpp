#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/logging.hpp>

#include <fmt/format.h>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class GuiLayoutGrid
        : public minire::GuiApplication
    {
        using GuiApplication::GuiApplication;

    protected:
        void onStart() override
        {
            GuiApplication::onStart();

            auto layout = std::make_shared<minire::gui::layouts::Grid>(3, 4);
            auto container = guiRoot().emplace<minire::gui::Component>("container");
            container->layout() = layout;

            for(size_t row = 0; row < layout->rows(); row++)
            {
                for(size_t col = 0; col < layout->cols(); col++)
                {
                    auto image = container->emplace<minire::gui::components::Image>(
                        fmt::format("cell-{}x{}", row, col),
                        minire::models::sprite::Image("image.png"));
                    image->vertical() = minire::gui::Arranger(minire::gui::position::Center{},
                                                              minire::gui::dimension::Content{});
                    image->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                                minire::gui::dimension::Content{});
                    layout->set(row, col, image->id());
                }
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
        GuiLayoutGrid application(1280, 720, "GUI Layout Grid", manager);
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
