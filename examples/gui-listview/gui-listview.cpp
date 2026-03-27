#include <minire/content/manager.hpp>
#include <minire/gui-application.hpp>
#include <minire/gui/components/listview.hpp>
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

    class GuiViewlist
        : public minire::GuiApplication
    {
    public:
        template<typename ... Args>
        GuiViewlist(Args &&... args)
            : GuiApplication(std::forward<Args>(args)...)
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

    protected:
        void onStart() override
        {
            GuiApplication::onStart();

            // base container

            auto layout = std::make_shared<layouts::Grid>(1, _cases.size());
            auto container = guiRoot().emplace<Component>("container");
            container->layout() = layout;

            // cases

            for(size_t column = 0; column < _cases.size(); ++column)
            {
                auto listview = container->emplace<ListView>(
                    fmt::format("listview-{}", column),
                    listview::SimpleItemBuilder(
                        [](std::any const & value, size_t const)
                        {
                            return minire::text::FormattedString(
                                std::any_cast<std::string>(value),
                                minire::text::Format().background(glm::vec4(0, 0, 0, 0))
                                                      .foreground(glm::vec4(0, 0, 0, 1)));
                        }));

                listview->horizontal()  = Arranger(position::Center{}, dimension::Constant{250});
                listview->vertical()    = Arranger(position::Center{}, dimension::Constant{100});
                *(listview->contents()) = _cases[column];

                listview->setCallback(std::in_place_type<listview::OnSelectionChanged>, "foo",
                    [](Component const & component, listview::OnSelectionChanged const & e)
                    {
                        MINIRE_INFO("Selection changed for \"{}\": {} -> {}",
                                    component.id(), e._previous, e._current);
                    });

                layout->set(0, column, listview->id());
            }
        }

    private:
        std::vector<std::vector<std::any>> const _cases;
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

        auto lease = manager.upload(kFontFace, minire::models::FontFace
            {
                ._regular = "../common/6x13.bdf",
                ._bold = "../common/6x13B.bdf",
                ._italic = "../common/6x13O.bdf",
                ._glyphWidth = 6,
                ._glyphHeight = 13,
            });

        GuiViewlist application(1280, 720, "GUI Dropdown", manager);
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
