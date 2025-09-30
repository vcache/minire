#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/button.hpp>
#include <minire/gui/components/container.hpp>
#include <minire/gui/components/image.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/logging.hpp>
#include <minire/models/font-face.hpp>

#include <fmt/format.h>

#include <cstdlib> // for EXIT_SUCCESS

namespace
{
    static std::string const kAtlas = "gui-atlas.png";
    static std::string const kFontFace = "ucs-6x13-example";

    class GuiButton
        : public minire::GuiController
    {
        using GuiController::GuiController;

    protected:

        void handle(minire::events::application::OnKeyDown const & e)
        {
            GuiController::handle(e);

            if (e._key == SDLK_TAB)
            {
                using namespace minire::gui::components;
                for(auto & component : _buttons)
                {
                    auto & button = component->as<minire::gui::components::Button>();
                    if (button.contentMargin()._left == 0)
                    {
                        button.setContentMargin(minire::utils::Rect(10.0f));
                    }
                    else
                    {
                        button.setContentMargin(minire::utils::Rect(0.0f));
                    }
                }
            }
        }

        void start() override
        {
            GuiController::start();

            using namespace minire::gui;
            using namespace minire::gui::components;
            using namespace minire::utils;

            // create test cases

            Button::Background const kBg
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

            std::vector<Arrangers> const kArrangers
            {
                Arrangers
                {
                    ._horizontal = Arranger(position::Center{}, dimension::Constant{0}),
                    ._vertical = Arranger(position::Center{}, dimension::Constant{0}),
                },
                Arrangers
                {
                    ._horizontal = Arranger(position::Center{}, dimension::Constant{75}),
                    ._vertical = Arranger(position::Center{}, dimension::Constant{45}),
                },
                Arrangers
                {
                    ._horizontal = Arranger(position::Center{}, dimension::Constant{45}),
                    ._vertical = Arranger(position::Center{}, dimension::Constant{75}),
                },
                Arrangers
                {
                    ._horizontal = Arranger(position::Center{}, dimension::Fraction{.5f}),
                    ._vertical = Arranger(position::Center{}, dimension::Fraction{.5f}),
                },
                Arrangers
                {
                    ._horizontal = Arranger(position::Center{}, dimension::Fill{}),
                    ._vertical = Arranger(position::Center{}, dimension::Fill{}),
                },
                Arrangers
                {
                    ._horizontal = Arranger(position::Center{}, dimension::Content{}),
                    ._vertical = Arranger(position::Center{}, dimension::Content{}),
                },
                Arrangers
                {
                    ._horizontal = Arranger(position::Center{}, dimension::Fraction{.5}),
                    ._vertical = Arranger(position::Center{}, dimension::Fraction{.5}),
                },
            };

            // build grid container
            auto layout = std::make_shared<layouts::Grid>(3 + 4*2, kArrangers.size());
            auto container = guiRoot().emplace<Container>("container", layout);

            for(size_t row = 0; row < layout->rows(); row++)
            {
                for(size_t col = 0; col < layout->cols(); col++)
                {
                    auto subContainer = container->emplace<Container>(
                        fmt::format("{}x{}", row, col));

                    subContainer->emplace<Image>(
                        fmt::format("bg", row, col), kAtlas,
                        NinePatch
                        {
                            ._boundary = Rect(30, 36, 51, 57),
                            ._out = Rect(33, 39, 48, 54),
                            ._in = Rect(36, 42, 45, 51),
                        });

                    layout->set(row, col, subContainer->id());
                }
            }

            // create buttons

            minire::text::FormattedString caption;
            caption.append(L"New").background(glm::vec4(0, 0, 0, 0))
                                  .foreground(glm::vec4(0, 0, 0, 1));

            Rect const kIconRect(46, 7, 56, 19);

            for(size_t i = 0; i < kArrangers.size(); ++i)
            {
                size_t row = 0;

                bool const isCheckable = i == 6;

                // empty content
                {
                    std::string cellId = fmt::format("{}x{}", row++, i);
                    auto button = container->at<Container>(cellId)
                        .emplace<Button>("btn", kBg, std::nullopt, std::nullopt, kArrangers[i], isCheckable);
                    button->setClickCallback([cellId](Button &) { MINIRE_INFO("Clicked at {}", cellId); } );
                    button->setCheckedCallback([cellId](models::Checkable & c)
                                               { MINIRE_INFO("Check at {}: {}", cellId, c.checked()); } );
                    _buttons.push_back(button);
                }

                // icon only
                {
                    Button::Icon icon {kAtlas, kIconRect};
                    std::string cellId = fmt::format("{}x{}", row++, i);
                    auto button = container->at<Container>(cellId)
                        .emplace<Button>("btn", kBg, icon, std::nullopt, kArrangers[i], isCheckable);
                    button->setClickCallback([cellId](Button &) { MINIRE_INFO("Clicked at {}", cellId); } );
                    button->setCheckedCallback([cellId](models::Checkable & c)
                                               { MINIRE_INFO("Check at {}: {}", cellId, c.checked()); } );
                    _buttons.push_back(button);
                }

                // text only
                {
                    Button::Text text
                    {
                        ._fontFace = kFontFace,
                        ._text = caption,
                    };
                    std::string cellId = fmt::format("{}x{}", row++, i);
                    auto button = container->at<Container>(cellId)
                        .emplace<Button>("btn", kBg, std::nullopt, text, kArrangers[i], isCheckable);
                    button->setClickCallback([cellId](Button &) { MINIRE_INFO("Clicked at {}", cellId); } );
                    button->setCheckedCallback([cellId](models::Checkable & c)
                                               { MINIRE_INFO("Check at {}: {}", cellId, c.checked()); } );
                    _buttons.push_back(button);
                }

                // icon + text
                for(Button::Icon::Position position : { Button::Icon::Position::kLeft,
                                                        Button::Icon::Position::kTop,
                                                        Button::Icon::Position::kRight,
                                                        Button::Icon::Position::kBottom})
                {
                    for(float const spacing : {0.0f, 10.0f})
                    {
                        Button::Icon icon
                        {
                            ._texture = kAtlas,
                            ._rect = kIconRect,
                            ._spacing = spacing,
                            ._position = position,
                        };

                        Button::Text text
                        {
                            ._fontFace = kFontFace,
                            ._text = caption,
                        };

                        std::string cellId = fmt::format("{}x{}", row++, i);
                        auto button = container->at<Container>(cellId)
                            .emplace<Button>("btn", kBg, icon, text, kArrangers[i], isCheckable);
                        button->setClickCallback([cellId](Button &) { MINIRE_INFO("Clicked at {}", cellId); } );
                        button->setCheckedCallback([cellId](models::Checkable & c)
                                                   { MINIRE_INFO("Check at {}: {}", cellId, c.checked()); } );
                        _buttons.push_back(button);
                    }
                }
            }
        }

    private:
        std::vector<minire::gui::Component::Sptr> _buttons;
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

        auto lease = manager.upload(kFontFace, minire::models::FontFace
            {
                ._regular = "../common/6x13.bdf",
                ._bold = "../common/6x13B.bdf",
                ._italic = "../common/6x13O.bdf",
                ._glyphWidth = 6,
                ._glyphHeight = 13,
            });

        minire::Application application(1280, 720, "GUI Button", manager);
        application.setController<GuiButton>(kMaxCtrlFps);
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
