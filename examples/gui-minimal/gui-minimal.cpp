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
                "center-center-example", makeImageView("center-center.png"));

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "center-less-example", makeImageView("center-less.png"));
                comp->horizontal()->_position = minire::gui::position::Center{};
                comp->vertical()->_position = minire::gui::position::Begin{};
                comp->vertical()->_marginMin = 10;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "center-more-example", makeImageView("center-more.png"));
                comp->horizontal()->_position = minire::gui::position::Center{};
                comp->vertical()->_position = minire::gui::position::End{};
                comp->vertical()->_marginMax = 10;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "less-center-example", makeImageView("less-center.png"));
                comp->horizontal()->_position = minire::gui::position::Begin{};
                comp->horizontal()->_marginMin = 10;
                comp->vertical()->_position = minire::gui::position::Center{};
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "more-center-example", makeImageView("more-center.png"));
                comp->horizontal()->_position = minire::gui::position::End{};
                comp->horizontal()->_marginMax = 10;
                comp->vertical()->_position = minire::gui::position::Center{};
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "less-less-example", makeImageView("less-less.png"));
                comp->horizontal()->_position = minire::gui::position::Begin{};
                comp->horizontal()->_marginMin = 100;
                comp->vertical()->_position = minire::gui::position::Begin{};
                comp->vertical()->_marginMin = 100;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "less-more-example", makeImageView("less-more.png"));
                comp->horizontal()->_position = minire::gui::position::Begin{};
                comp->horizontal()->_marginMin = 100;
                comp->vertical()->_position = minire::gui::position::End{};
                comp->vertical()->_marginMax = 100;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "more-less-example", makeImageView("more-less.png"));
                comp->horizontal()->_position = minire::gui::position::End{};
                comp->horizontal()->_marginMax = 100;
                comp->vertical()->_position = minire::gui::position::Begin{};
                comp->vertical()->_marginMin = 100;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "more-more-example", makeImageView("more-more.png"));
                comp->horizontal()->_position = minire::gui::position::End{};
                comp->horizontal()->_marginMax = 100;
                comp->vertical()->_position = minire::gui::position::End{};
                comp->vertical()->_marginMax = 100;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "origin-example", makeImageView("origin.png"));
                comp->horizontal() =  minire::gui::Arranger(minire::gui::position::Constant{0});
                comp->vertical() = minire::gui::Arranger(minire::gui::position::Constant{0});
            }

            _bottomRight = guiRoot().emplace<minire::gui::components::Image>(
                "bottom-right-example", makeImageView("bottom-right.png"));
            _bottomRight->horizontal() = minire::gui::Arranger(minire::gui::position::Constant{0});
            _bottomRight->vertical() = minire::gui::Arranger(minire::gui::position::Constant{0});
        }

        void handle(minire::events::application::OnResize const & e) override
        {
            GuiController::handle(e);

            assert(_bottomRight);

            _bottomRight->horizontal()->_position =
                minire::gui::position::Constant{static_cast<float>(e._width - 50)};
            _bottomRight->vertical()->_position =
                minire::gui::position::Constant{static_cast<float>(e._height - 50)};
        }

    private:
        minire::gui::Component::Sptr _bottomRight;
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
