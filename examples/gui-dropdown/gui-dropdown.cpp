#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/container.hpp>
#include <minire/gui/components/dropdown.hpp>
#include <minire/gui/components/scrollbar.hpp>
#include <minire/gui/components/text.hpp>
#include <minire/gui/layouts/grid.hpp>
#include <minire/logging.hpp>
#include <minire/models/font-face.hpp>
#include <minire/text/unicode.hpp>

#include <fmt/format.h>
#include <fmt/std.h>

#include <cstdlib> // for EXIT_SUCCESS

using namespace minire::gui;
using namespace minire::gui::components;
using namespace minire::utils;

namespace
{
    static std::string const kAtlas = "gui-atlas.png";
    static std::string const kFontFace = "ucs-6x13-example";

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

    Dropdown::Background const kBaseBg
    {
        ._texture = kAtlas,
        ._patch = NinePatch
        {
            ._boundary = Rect(83, 5, 103, 29),
            ._out = Rect(86, 8, 100, 26),
            ._in = Rect(89, 11, 97, 23),
        }
    };

    Dropdown::Background const kTongueBg
    {
        ._texture = kAtlas,
        ._patch = NinePatch
        {
            ._boundary = Rect(73, 37, 100, 64),
            ._out = Rect(75, 39, 98, 62),
            ._in = Rect(78, 42, 95, 59),
        }
    };

    Scrollbar::Background const kScrollbarBg
    {
        ._texture = kAtlas,
        ._patch = NinePatch
        {
            ._boundary = Rect(165, 29, 184, 47),
            ._out = Rect(167, 31, 182, 45),
            ._in = Rect(170, 34, 179, 42),
        }
    };

    Rect const kUpIcon(130, 47, 148, 65);
    Rect const kDownIcon(130, 23, 148, 41);

    Button::Background const kItemButtonBg
    {
        ._texture = kAtlas,
        ._normal = NinePatch
        {
            ._boundary = Rect(39, 53, 58, 72),
            ._out = Rect(41, 55, 56, 70),
            ._in = Rect(44, 58, 53, 67),
        },
        ._hovered = NinePatch
        {
            ._boundary = Rect(39, 76, 58, 95),
            ._out = Rect(41, 78, 56, 93),
            ._in = Rect(44, 81, 53, 90),
        },
        ._pressed = NinePatch
        {
            ._boundary = Rect(39, 99, 58, 118),
            ._out = Rect(41, 101, 56, 116),
            ._in = Rect(44, 104, 53, 113),
        },
    };

    class GuiDropdown
        : public minire::GuiController
    {
    public:
        template<typename ... Args>
        GuiDropdown(Args &&... args)
            : GuiController(std::forward<Args>(args)...)
            , _cases
            {
                std::vector<std::any>{},
                std::vector<std::any>
                {
                    std::string("foo"),
                    std::string("buz"),
                    std::string("bar"),
                },
                std::vector<std::any>
                {
                    std::string("1 - one"),
                    std::string("2 - two"),
                    std::string("3 - three"),
                    std::string("4 - four"),
                    std::string("5 - five"),
                    std::string("6 - six"),
                    std::string("7 - seven"),
                    std::string("8 - eight"),
                    std::string("9 - nine"),
                    std::string("10 - ten"),
                    std::string("11 - elleven"),
                    std::string("12 - twelve"),
                    std::string("13 - thirteen"),
                    std::string("14 - fourteen"),
                    std::string("15 - fifteen"),
                    std::string("16 - sixteen"),
                    std::string("17 - seventeen"),
                    std::string("18 - eighteen"),
                    std::string("19 - nineteen"),
                    std::string("20 - twenty"),
                }
            }
        {}

    private:
        void fillContents()
        {
            MINIRE_INFO("Seelcted case is: {}", _case);

            for(Dropdown::Sptr const & dropdown : _dropdowns)
            {
                assert(dropdown);
                dropdown->editContents(
                    [this](std::vector<std::any> & contents)
                    {
                        contents = _cases[_case];
                        return true;
                    });
            }

            _case = (_case + 1) % _cases.size();
        }

        Button::Sptr makeButton(std::string const & text,
                                Arrangers const & arrangers)
        {
            minire::text::FormattedString caption;
            caption.append(minire::text::toUnicode(text)).background(glm::vec4(0, 0, 0, 0))
                                                         .foreground(glm::vec4(0, 0, 0, 1));
            return std::make_shared<Button>(
                *this, text /*id*/, Container::Sptr(), kItemButtonBg, std::nullopt,
                Button::Text{kFontFace, caption}, arrangers);
        }

    protected:
        void start() override
        {
            GuiController::start();

            // base container

            auto layout = std::make_shared<layouts::Grid>(2, 2);
            auto container = guiRoot().emplace<Container>("container", layout);
            container->setArrangers(Arrangers::fill());

            // automatic heights case

            {
                auto dropdown = container->emplace<Dropdown>("automatic-heights");
                layout->set(0, 0, dropdown->id());
                dropdown->init(kBaseBg, kTongueBg, kButtonBg,
                    Button::Icon{kAtlas, Rect(38, 23, 56, 41)}, std::nullopt,
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Constant{200}),
                        ._vertical   = Arranger(position::Center{}, dimension::Constant{25}),
                    });

                Arrangers buttonArrangers = dropdown->button().arrangers();
                buttonArrangers._horizontal.setMarginMax(3);
                dropdown->button().setArrangers(buttonArrangers);

                dropdown->setItemBuilderCallback(
                    [this](std::any const & value, size_t, bool, Dropdown::Purpose) -> Button::Sptr
                    {
                        return makeButton(std::any_cast<std::string>(value),
                            Arrangers
                            {
                                ._horizontal = Arranger(position::Center{}, dimension::Fill{}, 5, 5),
                                ._vertical   = Arranger(position::Center{}, dimension::Fill{}),
                            });
                    });
                dropdown->setSelectionChangedCallback(
                    [](Dropdown const &, std::optional<size_t> previous, std::optional<size_t> current)
                    {
                        MINIRE_INFO("Selection changed from {} to {}", previous, current);
                    });
                dropdown->setScrollbarBuilderCallback(17, [this]()
                    {
                        auto scrollbar = std::make_shared<Scrollbar>(*this, "scrollbar", nullptr);
                        scrollbar->init(true, kScrollbarBg,
                            std::make_shared<Button>(*this, "down", nullptr, kButtonBg, Button::Icon{kAtlas, kDownIcon}),
                            std::make_shared<Button>(*this, "up", nullptr, kButtonBg, Button::Icon{kAtlas, kUpIcon}),
                            std::make_shared<Button>(*this, "slider", nullptr, kButtonBg));
                        return scrollbar;
                    });
                _dropdowns.push_back(dropdown);
            }

            // fixed heights case

            {
                auto dropdown = container->emplace<Dropdown>("fixed-heights");
                layout->set(0, 1, dropdown->id());
                dropdown->init(kBaseBg, kTongueBg, kButtonBg,
                    Button::Icon{kAtlas, Rect(38, 23, 56, 41)}, std::nullopt,
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Constant{200}),
                        ._vertical   = Arranger(position::Center{}, dimension::Constant{25}),
                    });

                Arrangers buttonArrangers = dropdown->button().arrangers();
                buttonArrangers._horizontal.setMarginMax(3);
                dropdown->button().setArrangers(buttonArrangers);

                dropdown->setItemBuilderCallback(
                    [this](std::any const & value, size_t, bool, Dropdown::Purpose) -> Button::Sptr
                    {
                        return makeButton(std::any_cast<std::string>(value),
                            Arrangers
                            {
                                ._horizontal = Arranger(position::Center{}, dimension::Fill{}, 5, 5),
                                ._vertical   = Arranger(position::Center{}, dimension::Constant{20})
                            });
                    });
                dropdown->setSelectionChangedCallback(
                    [](Dropdown const &, std::optional<size_t> previous, std::optional<size_t> current)
                    {
                        MINIRE_INFO("Selection changed from {} to {}", previous, current);
                    });
                dropdown->setScrollbarBuilderCallback(17, [this]()
                    {
                        auto scrollbar = std::make_shared<Scrollbar>(*this, "scrollbar", nullptr);
                        scrollbar->init(true, kScrollbarBg,
                            std::make_shared<Button>(*this, "down", nullptr, kButtonBg, Button::Icon{kAtlas, kDownIcon}),
                            std::make_shared<Button>(*this, "up", nullptr, kButtonBg, Button::Icon{kAtlas, kUpIcon}),
                            std::make_shared<Button>(*this, "slider", nullptr, kButtonBg));
                        return scrollbar;
                    });
                _dropdowns.push_back(dropdown);
            }

            // height from content case

            {
                auto dropdown = container->emplace<Dropdown>("content-defined-heights");
                layout->set(1, 0, dropdown->id());
                dropdown->init(kBaseBg, kTongueBg, kButtonBg,
                    Button::Icon{kAtlas, Rect(38, 23, 56, 41)}, std::nullopt,
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Constant{200}),
                        ._vertical   = Arranger(position::Center{}, dimension::Constant{25}),
                    });

                Arrangers buttonArrangers = dropdown->button().arrangers();
                buttonArrangers._horizontal.setMarginMax(3);
                dropdown->button().setArrangers(buttonArrangers);

                dropdown->setItemBuilderCallback(
                    [this](std::any const & value, size_t, bool, Dropdown::Purpose) -> Button::Sptr
                    {
                        return makeButton(std::any_cast<std::string>(value),
                            Arrangers
                            {
                                ._horizontal = Arranger(position::Center{}, dimension::Fill{}, 5, 5),
                                ._vertical   = Arranger(position::Center{}, dimension::Content{})
                            });
                    });
                dropdown->setSelectionChangedCallback(
                    [](Dropdown const &, std::optional<size_t> previous, std::optional<size_t> current)
                    {
                        MINIRE_INFO("Selection changed from {} to {}", previous, current);
                    });
                dropdown->setScrollbarBuilderCallback(17, [this]()
                    {
                        auto scrollbar = std::make_shared<Scrollbar>(*this, "scrollbar", nullptr);
                        scrollbar->init(true, kScrollbarBg,
                            std::make_shared<Button>(*this, "down", nullptr, kButtonBg, Button::Icon{kAtlas, kDownIcon}),
                            std::make_shared<Button>(*this, "up", nullptr, kButtonBg, Button::Icon{kAtlas, kUpIcon}),
                            std::make_shared<Button>(*this, "slider", nullptr, kButtonBg));
                        return scrollbar;
                    });
                _dropdowns.push_back(dropdown);
            }

            // constant line height case
            {
                auto dropdown = container->emplace<Dropdown>("constant-line-height");
                layout->set(1, 1, dropdown->id());
                dropdown->init(kBaseBg, kTongueBg, kButtonBg,
                    Button::Icon{kAtlas, Rect(38, 23, 56, 41)}, std::nullopt,
                    Arrangers
                    {
                        ._horizontal = Arranger(position::Center{}, dimension::Constant{200}),
                        ._vertical   = Arranger(position::Center{}, dimension::Constant{25}),
                    }, 200, 5, 35 /* constant line height */);

                Arrangers buttonArrangers = dropdown->button().arrangers();
                buttonArrangers._horizontal.setMarginMax(3);
                dropdown->button().setArrangers(buttonArrangers);

                dropdown->setItemBuilderCallback(
                    [this](std::any const & value, size_t, bool, Dropdown::Purpose) -> Button::Sptr
                    {
                        return makeButton(std::any_cast<std::string>(value),
                            Arrangers
                            {
                                ._horizontal = Arranger(position::Center{}, dimension::Fill{}, 5, 5),
                                ._vertical   = Arranger(position::Center{}, dimension::Content{}),
                            });
                    });
                dropdown->setSelectionChangedCallback(
                    [](Dropdown const &, std::optional<size_t> previous, std::optional<size_t> current)
                    {
                        MINIRE_INFO("Selection changed from {} to {}", previous, current);
                    });
                dropdown->setScrollbarBuilderCallback(17, [this]()
                    {
                        auto scrollbar = std::make_shared<Scrollbar>(*this, "scrollbar", nullptr);
                        scrollbar->init(true, kScrollbarBg,
                            std::make_shared<Button>(*this, "down", nullptr, kButtonBg, Button::Icon{kAtlas, kDownIcon}),
                            std::make_shared<Button>(*this, "up", nullptr, kButtonBg, Button::Icon{kAtlas, kUpIcon}),
                            std::make_shared<Button>(*this, "slider", nullptr, kButtonBg));
                        return scrollbar;
                    });
                _dropdowns.push_back(dropdown);
            }

            set(SDL_SCANCODE_TAB, 0, [this](::SDL_Scancode, uint16_t) -> bool
                {
                    fillContents();
                    return true;
                });

            fillContents();
        }

    private:
        std::vector<Dropdown::Sptr>              _dropdowns;
        std::vector<std::vector<std::any>> const _cases;
        size_t                                   _case = 0;
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

        minire::Application application(1280, 720, "GUI Dropdown", manager);
        application.setController<GuiDropdown>(kMaxCtrlFps);
        application.setVsync(true);
        application.setGlDebug(false);

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
