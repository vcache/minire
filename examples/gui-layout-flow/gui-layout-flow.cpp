#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/layouts/flow.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/logging.hpp>

#include <fmt/format.h>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    class GuiLayoutFlow
        : public minire::GuiController
    {
        using GuiController::GuiController;

    protected:
        void start() override
        {
            GuiController::start();

            using namespace minire::gui::layouts;

            auto baseLayout = std::make_shared<Grid>(2, 2);
            auto container = guiRoot().emplace<minire::gui::Component>("container");
            container->layout() = baseLayout;

            {
                // Horizontal, no spacing
                auto layout = std::make_shared<Flow>(
                    true,
                    std::list<Flow::Element>
                    {
                        Flow::Component{"first"},
                        Flow::Component{"second"},
                        Flow::Component{"third"},
                    });
                auto sample = container->emplace<minire::gui::Component>("sample-1");
                sample->layout() = layout;

                for(auto const & id : {"first", "second", "third"})
                {
                    auto image = sample->emplace<minire::gui::components::Image>(
                        id, makeImageView("image.png"));
                    image->vertical() = minire::gui::Arranger(minire::gui::position::Center{},
                                                              minire::gui::dimension::Content{});
                    image->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                                minire::gui::dimension::Content{});
                }

                baseLayout->set(0, 0, sample->id());
            }

            {
                // Horizontal, with spacing
                auto layout = std::make_shared<Flow>(
                    true,
                    std::list<Flow::Element>
                    {
                        Flow::Component{"first"},
                        Flow::Spacing(10),
                        Flow::Component{"second"},
                        Flow::Spacing(20),
                        Flow::Component{"third"},
                    });
                auto sample = container->emplace<minire::gui::Component>("sample-2");
                sample->layout() = layout;

                for(auto const & id : {"first", "second", "third"})
                {
                    auto image = sample->emplace<minire::gui::components::Image>(
                        id, makeImageView("image.png"));
                    image->vertical() = minire::gui::Arranger(minire::gui::position::Center{},
                                                              minire::gui::dimension::Content{});
                    image->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                                minire::gui::dimension::Content{});
                }

                baseLayout->set(1, 0, sample->id());
            }

            {
                // Vertical, no spacing
                auto layout = std::make_shared<Flow>(false);
                layout->pushBack(Flow::Component{"first"});
                layout->pushBack(Flow::Component{"second"});
                layout->pushBack(Flow::Component{"third"});

                auto sample = container->emplace<minire::gui::Component>("sample-3");
                sample->layout() = layout;

                for(auto const & id : {"first", "second", "third"})
                {
                    auto image = sample->emplace<minire::gui::components::Image>(
                        id, makeImageView("image.png"));
                    image->vertical() = minire::gui::Arranger(minire::gui::position::Center{},
                                                              minire::gui::dimension::Content{});
                    image->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                                minire::gui::dimension::Content{});
                }

                baseLayout->set(0, 1, sample->id());
            }

            {
                // Vertical, with spacing
                auto layout = std::make_shared<Flow>(false);
                layout->pushFront(Flow::Component{"third"});
                layout->pushFront(Flow::Spacing(20));
                layout->pushFront(Flow::Component{"second"});
                layout->pushFront(Flow::Spacing(10));
                layout->pushFront(Flow::Component{"first"});

                auto sample = container->emplace<minire::gui::Component>("sample-4");
                sample->layout() = layout;

                for(auto const & id : {"first", "second", "third"})
                {
                    auto image = sample->emplace<minire::gui::components::Image>(
                        id, makeImageView("image.png"));
                    image->vertical() = minire::gui::Arranger(minire::gui::position::Center{},
                                                              minire::gui::dimension::Content{});
                    image->horizontal() = minire::gui::Arranger(minire::gui::position::Center{},
                                                                minire::gui::dimension::Content{});
                }

                baseLayout->set(1, 1, sample->id());
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
        minire::Application application(1280, 720, "GUI Flow Layout", manager);
        application.setController<GuiLayoutFlow>(kMaxCtrlFps);
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
