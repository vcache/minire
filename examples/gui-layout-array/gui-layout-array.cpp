#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/layouts/array.hpp>
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
            using namespace minire::gui;

            static minire::utils::NinePatch const kHud9P
            {
                ._boundary = minire::utils::Rect(8, 8, 111, 111),
                ._out = minire::utils::Rect(22, 22, 97, 97),
                ._in = minire::utils::Rect(25, 25, 94, 94),
            };

            auto baseLayout = std::make_shared<Grid>(5, 2);
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = baseLayout;

            for(auto [horizontal, column] : {std::make_pair(true, 0), std::make_pair(false, 1)})
            {
                size_t row = 0;

                {
                    auto layout = std::make_shared<Array>(
                        horizontal,
                        Array::Elements
                        {
                            Array::Element{"first", dimension::Content{}},
                            Array::Element{"second", dimension::Content{}},
                            Array::Element{"third", dimension::Content{}},
                        });
                    auto sample = container->emplace<Component>(
                        fmt::format("sample-{}-{}", column, row));
                    sample->layout() = layout;

                    for(auto const & id : {"first", "second", "third"})
                    {
                        auto image = sample->emplace<components::Image>(
                            id, makeImageView("image.png"));
                        image->vertical() = Arranger(position::Center{}, dimension::Content{});
                        image->horizontal() = Arranger(position::Center{}, dimension::Content{});
                    }

                    baseLayout->set(row++, column, sample->id());
                }

                {
                    auto layout = std::make_shared<Array>(
                        horizontal,
                        Array::Elements
                        {
                            Array::Element{"first", dimension::Content{}},
                            Array::Element{"second", dimension::Fill{}},
                            Array::Element{"third", dimension::Content{}},
                        });
                    auto sample = container->emplace<Component>(
                        fmt::format("sample-{}-{}", column, row));
                    sample->layout() = layout;

                    for(auto const & [id, pos] : {std::make_pair("first", Position(position::Begin{})),
                                                  std::make_pair("second", Position(position::Center{})),
                                                  std::make_pair("third", Position(position::End{}))})
                    {
                        auto image = sample->emplace<components::Image>(id, makeImageView("image.png"));
                        image->vertical() = Arranger(pos, dimension::Content{});
                        image->horizontal() = Arranger(pos, dimension::Content{});
                    }

                    baseLayout->set(row++, column, sample->id());
                }

                {
                    auto layout = std::make_shared<Array>(
                        horizontal,
                        Array::Elements
                        {
                            Array::Element{"first", dimension::Content{}},
                            Array::Element{"second", dimension::Fill{}},
                            Array::Element{"third", dimension::Content{}},
                        });
                    auto sample = container->emplace<Component>(
                        fmt::format("sample-{}-{}", column, row));
                    sample->layout() = layout;

                    sample->emplace<components::Image>("first", makeImageView("image.png"));
                    sample->emplace<components::Image>("second", makeImageView("hud.png", kHud9P));
                    sample->emplace<components::Image>("third", makeImageView("image.png"));

                    baseLayout->set(row++, column, sample->id());
                }

                {
                    auto layout = std::make_shared<Array>(
                        horizontal,
                        Array::Elements
                        {
                            Array::Element{"first", dimension::Content{}},
                            Array::Element{std::nullopt, dimension::Constant{20}},
                            Array::Element{"second", dimension::Fill{}},
                            Array::Element{std::nullopt, dimension::Constant{20}},
                            Array::Element{"third", dimension::Content{}},
                        });
                    auto sample = container->emplace<Component>(
                        fmt::format("sample-{}-{}", column, row));
                    sample->layout() = layout;

                    sample->emplace<components::Image>("first", makeImageView("image.png"));
                    sample->emplace<components::Image>("second", makeImageView("hud.png", kHud9P));
                    sample->emplace<components::Image>("third", makeImageView("image.png"));

                    baseLayout->set(row++, column, sample->id());
                }

                {
                    auto layout = std::make_shared<Array>(
                        horizontal,
                        Array::Elements
                        {
                            Array::Element{"first", dimension::Content{}},
                            Array::Element{std::nullopt, dimension::Fill{}},
                            Array::Element{"third", dimension::Content{}},
                        });
                    auto sample = container->emplace<Component>(
                        fmt::format("sample-{}-{}", column, row));
                    sample->layout() = layout;

                    sample->emplace<components::Image>("first", makeImageView("image.png"));
                    sample->emplace<components::Image>("third", makeImageView("image.png"));

                    baseLayout->set(row++, column, sample->id());
                }
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
