#include <minire/application.hpp>

#include <minire/content/manager.hpp>
#include <minire/gui-controller.hpp>
#include <minire/gui/components/dropdown.hpp>
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
    static std::string const kFontFace = "ucs-6x13-example";

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
                    std::string("very long ling that should "
                                "demonstrate clipping window"),
                },
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
                *dropdown->contents() = _cases[_case];
            }

            _case = (_case + 1) % _cases.size();
        }

        auto makeItem(std::string const & text)
        {
            using namespace minire::text;
            FormattedString caption(text, Format().background(glm::vec4(0, 0, 0, 0))
                                                  .foreground(glm::vec4(0, 0, 0, 1)));
            return makeTextView(caption, kFontFace);
        }

    protected:
        void start() override
        {
            GuiController::start();

            // base container

            auto layout = std::make_shared<layouts::Grid>(1, 2);
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = layout;

            // automatic heights case

            {
                auto dropdown = container->emplace<Dropdown>("automatic-heights");
                layout->set(0, 0, dropdown->id());
                dropdown->horizontal() = Arranger(position::Center{}, dimension::Constant{200});
                dropdown->vertical()   = Arranger(position::Center{}, dimension::Constant{25});
                dropdown->setBaseItemBuilder(
                    [this](std::any const & value, size_t) -> ContentView::Sptr
                    {
                        return makeItem(std::any_cast<std::string>(value));
                    });
                dropdown->setCallback(std::in_place_type<dropdown::OnSelectionChanged>, "foo",
                    [](Component const &, dropdown::OnSelectionChanged const & e)
                    {
                        MINIRE_INFO("Selection changed from {} to {}", e._previous, e._current);
                    });
                _dropdowns.push_back(dropdown);
            }

            // fixed heights case

            {
                auto dropdown = container->emplace<Dropdown>("fixed-heights");
                layout->set(0, 1, dropdown->id());
                dropdown->horizontal() = Arranger(position::Center{}, dimension::Constant{200});
                dropdown->vertical()   = Arranger(position::Center{}, dimension::Constant{25});
                dropdown->lineHeight() = 50;
                dropdown->setBaseItemBuilder(
                    [this](std::any const & value, size_t) -> ContentView::Sptr
                    {
                        return makeItem(std::any_cast<std::string>(value));
                    });
                dropdown->setCallback(std::in_place_type<dropdown::OnSelectionChanged>, "foo",
                    [](Component const &, dropdown::OnSelectionChanged const & e)
                    {
                        MINIRE_INFO("Selection changed from {} to {}", e._previous, e._current);
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
