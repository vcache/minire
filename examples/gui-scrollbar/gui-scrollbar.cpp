#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/container.hpp>
#include <minire/gui/components/scrollbar.hpp>
#include <minire/gui/components/button.hpp>
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
    static std::string const kAtlas = "gui-atlas.png";

    Button::Background const kButtonBg
    {
        ._texture = kAtlas,
        ._normal = NinePatch
            {
                ._boundary = Rect(1, 1, 27, 26),
                ._out = Rect(3, 3, 25, 24),
                ._in = Rect(6, 6, 22, 21),
            },
        ._hovered = NinePatch
            {
                ._boundary = Rect(1, 30, 27, 55),
                ._out = Rect(3, 32, 25, 53),
                ._in = Rect(6, 35, 22, 50),
            },
        ._pressed = NinePatch
            {
                ._boundary = Rect(1, 59, 27, 84),
                ._out = Rect(3, 61, 25, 82),
                ._in = Rect(6, 64, 22, 79),
            },
    };

    Scrollbar::Background const kBaseBg
    {
        ._texture = kAtlas,
        ._patch = NinePatch
        {
            ._boundary = Rect(73, 29, 92, 47),
            ._out = Rect(75, 31, 90, 45),
            ._in = Rect(78, 34, 87, 42),
        }
    };

    Rect const kUpIcon(38, 47, 56, 65);
    Rect const kDownIcon(38, 23, 56, 41);
    Rect const kLeftIcon(38, 1, 56, 19);
    Rect const kRightIcon(63, 1, 81, 19);

    class GuiScrollbar
        : public minire::GuiController
    {
        using GuiController::GuiController;

    protected:
        void start() override
        {
            GuiController::start();

            // base container

            auto layout = std::make_shared<layouts::Grid>(2, 4);
            auto container = guiRoot().emplace<Container>("container", layout);
            container->setArrangers(Arrangers::fill());

            {
                auto scrollbar = container->emplace<Scrollbar>("vert-without-buttons");
                layout->set(0, 0, scrollbar->id());
                scrollbar->init(true, kBaseBg, nullptr, nullptr,
                    std::make_shared<Button>(*this, "slider", nullptr, kButtonBg),
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Constant{50}),
                        ._vertical   = Arranger(position::Center{}, dimension::Fill{}, 10, 10),
                    });
                scrollbar->setValueChangedCallback(
                    [](Scrollbar const & scrollbar, float const prev, float const curr)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", scrollbar.id(), prev, curr); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("vert-with-buttons");
                layout->set(0, 1, scrollbar->id());
                scrollbar->init(true, kBaseBg,
                    std::make_shared<Button>(*this, "down", nullptr, kButtonBg, Button::Icon{kAtlas, kDownIcon},
                                             std::nullopt, Arrangers::fill()),
                    std::make_shared<Button>(*this, "up", nullptr, kButtonBg, Button::Icon{kAtlas, kUpIcon},
                                             std::nullopt, Arrangers::fill()),
                    std::make_shared<Button>(*this, "slider", nullptr, kButtonBg),
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Constant{50}),
                        ._vertical   = Arranger(position::Center{}, dimension::Fill{}, 10, 10),
                    });
                scrollbar->setValueChangedCallback(
                    [](Scrollbar const & scrollbar, float const prev, float const curr)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", scrollbar.id(), prev, curr); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("vert-inc-only");
                layout->set(0, 2, scrollbar->id());
                scrollbar->init(true, kBaseBg,
                    std::make_shared<Button>(*this, "down", nullptr, kButtonBg, Button::Icon{kAtlas, kDownIcon},
                                             std::nullopt, Arrangers::fill()),
                    nullptr,
                    std::make_shared<Button>(*this, "slider", nullptr, kButtonBg),
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Constant{50}),
                        ._vertical   = Arranger(position::Center{}, dimension::Fill{}, 10, 10),
                    });
                scrollbar->setValueChangedCallback(
                    [](Scrollbar const & scrollbar, float const prev, float const curr)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", scrollbar.id(), prev, curr); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("vert-dec-only");
                layout->set(0, 3, scrollbar->id());
                scrollbar->init(true, kBaseBg,
                    nullptr,
                    std::make_shared<Button>(*this, "up", nullptr, kButtonBg, Button::Icon{kAtlas, kUpIcon},
                                             std::nullopt, Arrangers::fill()),
                    std::make_shared<Button>(*this, "slider", nullptr, kButtonBg),
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Constant{50}),
                        ._vertical   = Arranger(position::Center{}, dimension::Fill{}, 10, 10),
                    });
                scrollbar->setValueChangedCallback(
                    [](Scrollbar const & scrollbar, float const prev, float const curr)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", scrollbar.id(), prev, curr); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("horiz-without-buttons");
                layout->set(1, 0, scrollbar->id());
                scrollbar->init(false, kBaseBg, nullptr, nullptr,
                    std::make_shared<Button>(*this, "slider", nullptr, kButtonBg),
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Fill{}, 10, 10),
                        ._vertical   = Arranger(position::Center{}, dimension::Constant{50}),
                    });
                scrollbar->setValueChangedCallback(
                    [](Scrollbar const & scrollbar, float const prev, float const curr)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", scrollbar.id(), prev, curr); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("horiz-with-buttons");
                layout->set(1, 1, scrollbar->id());
                scrollbar->init(false, kBaseBg,
                    std::make_shared<Button>(*this, "right", nullptr, kButtonBg, Button::Icon{kAtlas, kRightIcon},
                                             std::nullopt, Arrangers::fill()),
                    std::make_shared<Button>(*this, "left", nullptr, kButtonBg, Button::Icon{kAtlas, kLeftIcon},
                                             std::nullopt, Arrangers::fill()),
                    std::make_shared<Button>(*this, "slider", nullptr, kButtonBg),
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Fill{}, 10, 10),
                        ._vertical   = Arranger(position::Center{}, dimension::Constant{50}),
                    });
                scrollbar->setValueChangedCallback(
                    [](Scrollbar const & scrollbar, float const prev, float const curr)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", scrollbar.id(), prev, curr); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("horiz-inc-only");
                layout->set(1, 2, scrollbar->id());
                scrollbar->init(false, kBaseBg,
                    std::make_shared<Button>(*this, "right", nullptr, kButtonBg, Button::Icon{kAtlas, kRightIcon},
                                             std::nullopt, Arrangers::fill()),
                    nullptr,
                    std::make_shared<Button>(*this, "slider", nullptr, kButtonBg),
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Fill{}, 10, 10),
                        ._vertical   = Arranger(position::Center{}, dimension::Constant{50}),
                    });
                scrollbar->setValueChangedCallback(
                    [](Scrollbar const & scrollbar, float const prev, float const curr)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", scrollbar.id(), prev, curr); });
            }

            {
                auto scrollbar = container->emplace<Scrollbar>("horiz-dec-only");
                layout->set(1, 3, scrollbar->id());
                scrollbar->init(false, kBaseBg,
                    nullptr,
                    std::make_shared<Button>(*this, "left", nullptr, kButtonBg, Button::Icon{kAtlas, kLeftIcon},
                                             std::nullopt, Arrangers::fill()),
                    std::make_shared<Button>(*this, "slider", nullptr, kButtonBg),
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Fill{}, 10, 10),
                        ._vertical   = Arranger(position::Center{}, dimension::Constant{50}),
                    });
                scrollbar->setValueChangedCallback(
                    [](Scrollbar const & scrollbar, float const prev, float const curr)
                    { MINIRE_INFO("Value of \"{}\" changed {} -> {}", scrollbar.id(), prev, curr); });
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
