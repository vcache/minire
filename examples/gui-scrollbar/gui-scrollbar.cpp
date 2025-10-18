#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/scrollbar.hpp>
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
    class GuiScrollbar
        : public minire::GuiController
    {
        using GuiController::GuiController;

    protected:
        void start() override
        {
            GuiController::start();

            auto layout = std::make_shared<layouts::Grid>(2, 4);
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = layout;
            container->horizontal() = Arranger::fill();
            container->vertical() = Arranger::fill();

            {
                auto scrollbar = container->emplace<Scrollbar>("vert-without-buttons", true);
                layout->set(0, 0, scrollbar->id());
                scrollbar->increaseButton().visible() = false;
                scrollbar->decreaseButton().visible() = false;
                scrollbar->horizontal() = Arranger(position::Center{}, dimension::Constant{50});
                scrollbar->vertical()   = Arranger(position::Center{}, dimension::Fill{}, 10, 10);
                scrollbar->setCallback(std::in_place_type<scrollbar::OnValueChanged>, "foo",
                    [](Component & component, scrollbar::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("vert-with-buttons", true);
                layout->set(0, 1, scrollbar->id());
                scrollbar->horizontal() = Arranger(position::Center{}, dimension::Constant{50});
                scrollbar->vertical()   = Arranger(position::Center{}, dimension::Fill{}, 10, 10);
                scrollbar->setCallback(std::in_place_type<scrollbar::OnValueChanged>, "foo",
                    [](Component & component, scrollbar::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("vert-inc-only", true);
                layout->set(0, 2, scrollbar->id());
                scrollbar->horizontal() = Arranger(position::Center{}, dimension::Constant{50});
                scrollbar->vertical()   = Arranger(position::Center{}, dimension::Fill{}, 10, 10);
                scrollbar->decreaseButton().visible() = false;
                scrollbar->setCallback(std::in_place_type<scrollbar::OnValueChanged>, "foo",
                    [](Component & component, scrollbar::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("vert-dec-only", true);
                layout->set(0, 3, scrollbar->id());
                scrollbar->horizontal() = Arranger(position::Center{}, dimension::Constant{50});
                scrollbar->vertical()   = Arranger(position::Center{}, dimension::Fill{}, 10, 10);
                scrollbar->increaseButton().visible() = false;
                scrollbar->setCallback(std::in_place_type<scrollbar::OnValueChanged>, "foo",
                    [](Component & component, scrollbar::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("horiz-without-buttons", false);
                layout->set(1, 0, scrollbar->id());
                scrollbar->horizontal() = Arranger(position::Center{}, dimension::Fill{}, 10, 10);
                scrollbar->vertical()   = Arranger(position::Center{}, dimension::Constant{50});
                scrollbar->increaseButton().visible() = false;
                scrollbar->decreaseButton().visible() = false;
                scrollbar->setCallback(std::in_place_type<scrollbar::OnValueChanged>, "foo",
                    [](Component & component, scrollbar::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("horiz-with-buttons", false);
                layout->set(1, 1, scrollbar->id());
                scrollbar->horizontal() = Arranger(position::Center{}, dimension::Fill{}, 10, 10);
                scrollbar->vertical()   = Arranger(position::Center{}, dimension::Constant{50});
                scrollbar->setCallback(std::in_place_type<scrollbar::OnValueChanged>, "foo",
                    [](Component & component, scrollbar::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("horiz-inc-only", false);
                layout->set(1, 2, scrollbar->id());
                scrollbar->horizontal() = Arranger(position::Center{}, dimension::Fill{}, 10, 10);
                scrollbar->vertical()   = Arranger(position::Center{}, dimension::Constant{50});
                scrollbar->decreaseButton().visible() = false;
                scrollbar->setCallback(std::in_place_type<scrollbar::OnValueChanged>, "foo",
                    [](Component & component, scrollbar::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("horiz-dec-only", false);
                layout->set(1, 3, scrollbar->id());
                scrollbar->horizontal() = Arranger(position::Center{}, dimension::Fill{}, 10, 10);
                scrollbar->vertical()   = Arranger(position::Center{}, dimension::Constant{50});
                scrollbar->increaseButton().visible() = false;
                scrollbar->setCallback(std::in_place_type<scrollbar::OnValueChanged>, "foo",
                    [](Component & component, scrollbar::OnValueChanged const & e)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", component.id(), e._previous, e._current); });
            }
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
        minire::Application application(1280, 720, "GUI Scrollbar", manager);
        application.setController<GuiScrollbar>(kMaxCtrlFps);
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
