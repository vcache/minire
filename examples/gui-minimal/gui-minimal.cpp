#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/logging.hpp>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class GuiMinimal
        : public minire::GuiApplication
    {
        using GuiApplication::GuiApplication;

    protected:
        void onStart() override
        {
            GuiApplication::onStart();

            using namespace minire::models;

            guiRoot().emplace<minire::gui::components::Image>(
                "center-center-example", sprite::Image("center-center.png"));

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "center-less-example", sprite::Image("center-less.png"));
                comp->horizontal()->_position = minire::gui::position::Center{};
                comp->vertical()->_position = minire::gui::position::Begin{};
                comp->vertical()->_marginMin = 10;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "center-more-example", sprite::Image("center-more.png"));
                comp->horizontal()->_position = minire::gui::position::Center{};
                comp->vertical()->_position = minire::gui::position::End{};
                comp->vertical()->_marginMax = 10;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "less-center-example", sprite::Image("less-center.png"));
                comp->horizontal()->_position = minire::gui::position::Begin{};
                comp->horizontal()->_marginMin = 10;
                comp->vertical()->_position = minire::gui::position::Center{};
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "more-center-example", sprite::Image("more-center.png"));
                comp->horizontal()->_position = minire::gui::position::End{};
                comp->horizontal()->_marginMax = 10;
                comp->vertical()->_position = minire::gui::position::Center{};
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "less-less-example", sprite::Image("less-less.png"));
                comp->horizontal()->_position = minire::gui::position::Begin{};
                comp->horizontal()->_marginMin = 100;
                comp->vertical()->_position = minire::gui::position::Begin{};
                comp->vertical()->_marginMin = 100;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "less-more-example", sprite::Image("less-more.png"));
                comp->horizontal()->_position = minire::gui::position::Begin{};
                comp->horizontal()->_marginMin = 100;
                comp->vertical()->_position = minire::gui::position::End{};
                comp->vertical()->_marginMax = 100;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "more-less-example", sprite::Image("more-less.png"));
                comp->horizontal()->_position = minire::gui::position::End{};
                comp->horizontal()->_marginMax = 100;
                comp->vertical()->_position = minire::gui::position::Begin{};
                comp->vertical()->_marginMin = 100;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "more-more-example", sprite::Image("more-more.png"));
                comp->horizontal()->_position = minire::gui::position::End{};
                comp->horizontal()->_marginMax = 100;
                comp->vertical()->_position = minire::gui::position::End{};
                comp->vertical()->_marginMax = 100;
            }

            {
                auto comp = guiRoot().emplace<minire::gui::components::Image>(
                    "origin-example", sprite::Image("origin.png"));
                comp->horizontal() =  minire::gui::Arranger(minire::gui::position::Constant{0});
                comp->vertical() = minire::gui::Arranger(minire::gui::position::Constant{0});
            }

            _bottomRight = guiRoot().emplace<minire::gui::components::Image>(
                "bottom-right-example", sprite::Image("bottom-right.png"));
            _bottomRight->horizontal() = minire::gui::Arranger(minire::gui::position::Constant{0});
            _bottomRight->vertical() = minire::gui::Arranger(minire::gui::position::Constant{0});
        }

        bool handle(minire::application::OnResize const & e) override
        {
            if (GuiApplication::handle(e))
                return true;

            if (_bottomRight)
            {
                _bottomRight->horizontal()->_position =
                    minire::gui::position::Constant{static_cast<float>(e._width - 50)};
                _bottomRight->vertical()->_position =
                    minire::gui::position::Constant{static_cast<float>(e._height - 50)};
            }

            return true;
        }

    private:
        minire::gui::Component::Sptr _bottomRight;
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
        GuiMinimal application(1280, 720, "GUI Minimal", manager);
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
