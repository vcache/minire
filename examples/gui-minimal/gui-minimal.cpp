#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/logging.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class GuiMinimal
        : public minire::GuiController
    {
        using GuiController::GuiController;

    protected:
        void start() override
        {
            GuiController::start();

            guiRoot().emplace<minire::gui::components::Image>(
                "center-center-example", "center-center.png");

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "center-less-example", "center-less.png");
                auto arrangers = comp->arrangers();
                arrangers._horizontal.setPosition(minire::gui::position::Center{});
                arrangers._vertical.setPosition(minire::gui::position::Less{});
                arrangers._vertical.setMarginMin(10);
                comp->setArrangers(arrangers);
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "center-more-example", "center-more.png");
                auto arrangers = comp->arrangers();
                arrangers._horizontal.setPosition(minire::gui::position::Center{});
                arrangers._vertical.setPosition(minire::gui::position::More{});
                arrangers._vertical.setMarginMax(10);
                comp->setArrangers(arrangers);
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "less-center-example", "less-center.png");
                auto arrangers = comp->arrangers();
                arrangers._horizontal.setPosition(minire::gui::position::Less{});
                arrangers._horizontal.setMarginMin(10);
                arrangers._vertical.setPosition(minire::gui::position::Center{});
                comp->setArrangers(arrangers);
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "more-center-example", "more-center.png");
                auto arrangers = comp->arrangers();
                arrangers._horizontal.setPosition(minire::gui::position::More{});
                arrangers._horizontal.setMarginMax(10);
                arrangers._vertical.setPosition(minire::gui::position::Center{});
                comp->setArrangers(arrangers);
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "less-less-example", "less-less.png");
                auto arrangers = comp->arrangers();
                arrangers._horizontal.setPosition(minire::gui::position::Less{});
                arrangers._horizontal.setMarginMin(100);
                arrangers._vertical.setPosition(minire::gui::position::Less{});
                arrangers._vertical.setMarginMin(100);
                comp->setArrangers(arrangers);
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "less-more-example", "less-more.png");
                auto arrangers = comp->arrangers();
                arrangers._horizontal.setPosition(minire::gui::position::Less{});
                arrangers._horizontal.setMarginMin(100);
                arrangers._vertical.setPosition(minire::gui::position::More{});
                arrangers._vertical.setMarginMax(100);
                comp->setArrangers(arrangers);
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "more-less-example", "more-less.png");
                auto arrangers = comp->arrangers();
                arrangers._horizontal.setPosition(minire::gui::position::More{});
                arrangers._horizontal.setMarginMax(100);
                arrangers._vertical.setPosition(minire::gui::position::Less{});
                arrangers._vertical.setMarginMin(100);
                comp->setArrangers(arrangers);
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "more-more-example", "more-more.png");
                auto arrangers = comp->arrangers();
                arrangers._horizontal.setPosition(minire::gui::position::More{});
                arrangers._horizontal.setMarginMax(100);
                arrangers._vertical.setPosition(minire::gui::position::More{});
                arrangers._vertical.setMarginMax(100);
                comp->setArrangers(arrangers);
            }

            guiRoot().emplace<minire::gui::components::Image>(
                "origin-example", "origin.png", std::monostate(),
                minire::gui::Arrangers
                {
                    ._horizontal = minire::gui::Arranger(minire::gui::position::Constant{0}),
                    ._vertical = minire::gui::Arranger(minire::gui::position::Constant{0})
                });

            _bottomRight = guiRoot().emplace<minire::gui::components::Image>(
                "bottom-right-example", "bottom-right.png", std::monostate(),
                minire::gui::Arrangers
                {
                    ._horizontal = minire::gui::Arranger(minire::gui::position::Constant{0}),
                    ._vertical = minire::gui::Arranger(minire::gui::position::Constant{0})
                });
        }

        void handle(minire::events::application::OnResize const & e) override
        {
            GuiController::handle(e);

            assert(_bottomRight);

            auto arrangers = _bottomRight->arrangers();
            arrangers._horizontal.setPosition(
                minire::gui::position::Constant{static_cast<float>(e._width - 50)});
            arrangers._vertical.setPosition(
                minire::gui::position::Constant{static_cast<float>(e._height - 50)});
            _bottomRight->setArrangers(arrangers);
        }

    private:
        using ComponentSptr = minire::gui::Component::Sptr;
        ComponentSptr _bottomRight;
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
        minire::Application application(1280, 720, "GUI Minimal", manager);
        application.setController<GuiMinimal>(kMaxCtrlFps);
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
